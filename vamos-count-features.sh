#!/bin/sh

set -eu

#setup environment
config=$(dirname $0)/config.sh

if [ ! -f "$config" ]; then
    echo "fail to find config: $config"
    exit 1
fi

. "$config"

echo "Node $node processes $KERNEL_VERSIONS"

cd "${WORKDIR}"
echo "working in ${WORKDIR}"

if [ "`uname -m`" = "x86_64" ] && [ "`gcc -dumpmachine`" = "i486-linux-gnu" ]; then
    LINUX32=linux32
    ARCH=x86
    export ARCH
fi

if [ ! -d ${KERNELWORKDIR} ]; then
    mkdir -p ${KERNELWORKDIR}
fi
cd ${KERNELWORKDIR}

if [ ! -d ${KERNELWORKDIR}/linux ]; then
    echo "Fetching Linux sources from $KERNELGITURL"
    git clone $KERNELGITURL linux
fi

cd ${KERNELWORKDIR}/linux
git remote update || true
echo "Linux Sources ready in ${KERNELWORKDIR}/linux"

count_for_version () {
    version=$1

    if [ ! -d "${WORKDIR}/done" ]; then
	mkdir -p "${WORKDIR}/done/"
    fi
    rm -rf "${WORKDIR}/done/$v"

    echo "checking out version $version"
    git reset --hard
    git clean -fd >/dev/null 2>&1 || true
    git checkout $version
    git reset --hard
    git clean -fdx >/dev/null 2>&1 || true

    # prepare
    echo "Preparing build environment"
    touch scripts/kconfig/dumpconf.c
    touch scripts/kconfig/zconf.tab.o
    cp -v /proj/i4vamos/tools/tartler/Makefile.vamos .
    cp -v /proj/i4vamos/tools/tartler/postprocess.rml .
    cp -v /proj/i4vamos/tools/bin/dumpconf scripts/kconfig

    # cheap work first
    make allmodconfig silentoldconfig
    nice make -f Makefile.vamos all-arch-rsf RSFMODE=checker
    cp -v kconfig-x86.rsf "${WORKDIR}/${version}-kconfig-x86.rsf"

    find . -name 'Kconfig*' -exec grep -e '^config ' {} + > "${WORKDIR}/${version}-all-configs.list"
    find . -name '*.[c|h|S]' -type f ! -empty -ls > "${WORKDIR}/${version}-source-files.list"

    # now heavy work1: generate rsf files
    if [ ! -f "${WORKDIR}/${version}-rsf-files.cpio" ]; then
	echo "Extracting CPP Blocks from source files"
	nice make -f Makefile.vamos -j 5 source2rsf-stamp RSFMODE=checker >/dev/null 2>&1
	find . -name '*.rsf' | cpio -o > "${WORKDIR}/${version}-rsf-files.cpio"
    fi

    # check if we can move to tmpfs
    rm -rf "/dev/shm/`whoami`/${version}"
    INODESAVAIL=$(df -i /dev/shm | awk '/^(none|tmpfs)/ {print $4}')
    if [ "$INODESAVAIL" -gt 900000 ]; then
	echo "Working in /dev/shm/`whoami`/${version}"
	mkdir -p "/dev/shm/`whoami`/${version}"
	cd "/dev/shm/`whoami`/${version}"
	mkdir -p scripts/kconfig
	touch scripts/kconfig/dumpconf.c
	touch scripts/kconfig/zconf.tab.o
	cp -v /proj/i4vamos/tools/tartler/Makefile.vamos .
	cp -v /proj/i4vamos/tools/tartler/postprocess.rml .
	cp -v /proj/i4vamos/tools/bin/dumpconf scripts/kconfig
	cp -v ${KERNELWORKDIR}/linux/kconfig-*.rsf .
	cpio -id < "${WORKDIR}/${version}-rsf-files.cpio"
	touch source2rsf-stamp
    fi
    echo "CPP Block information ready"
    find . -name '*.rsf' -ls > "${WORKDIR}/${version}-rsf-files.list"

    if [ ! -f  "${WORKDIR}/${version}-rsfout-files.cpio" ]; then
	echo "Calculating ifdef clouds"
	nice make -f Makefile.vamos -j 5 crocopat-stamp RSFMODE=checker >/dev/null 2>&1
	find . -name '*.rsfout' -o -name '*.crocopat' \
	    | cpio -o > "${WORKDIR}/${version}-rsfout-files.cpio"
    else
	cpio -id < "${WORKDIR}/${version}-rsfout-files.cpio"
	touch crocopat-stamp
    fi

    echo "ifdef clouds calculated"
    find . -name '*.rsfout' -o -name '*.crocopat' -ls \
	> "${WORKDIR}/${version}-rsfout-files.list"
    find . -name '*.rsfout' -exec grep -E '^CondBlockIsAtPos' {} + \
	> "${WORKDIR}/${version}-condblock.list"

    #kernel < 2.6.26 had amd64 and i386 seperated
    if [ ! -r kconfig-x86.rsf ]; then
	continue
    fi

    if [ ! -f "${WORKDIR}/${version}-codesat-files.cpio" ]; then
	echo "Generating Variability model"
	nice time make -f Makefile.vamos sat-stamp RSFMODE=checker
	find -name '*.codesat' | cpio -o > "${WORKDIR}/${version}-codesat-files.cpio"
    else
	cpio -id < "${WORKDIR}/${version}-codesat-files.cpio"
    fi

    echo "Variability model ready"
    find . -name '*.codesat' > sat-stamp
    find -name '*.codesat' -ls  > "${WORKDIR}/${version}-codesat.files"

    if [ ! -f "${WORKDIR}/${version}-dead-files.cpio" ]; then
	echo "Executing the queries"
	cat /proj/i4vamos/tools/`whoami`/whitelist.d/* \
	    | grep -v -E '^#' | sort | uniq > whitelist
	printf "whitelist has %s entries\n" `wc -l whitelist`
	nice undertaker2 -w whitelist
	find . -name '*.dead' | cpio -o > "${WORKDIR}/${version}-dead-files.cpio"
    else
	cpio -id < "${WORKDIR}/${version}-dead-files.cpio"
    fi

    echo "Postprocessing"
    cp whitelist "${WORKDIR}/${version}-whitelist.txt"
    find . -name '*.dead' > "${WORKDIR}/${version}-dead-files.list"
    find . -type f -ls > "${WORKDIR}/${version}-allfiles-files.list"
    rm -rf "/dev/shm/`whoami`/${version}"
    touch "${WORKDIR}/done/$v"
}

for v in $KERNEL_VERSIONS; do
    echo "running for version $v"
    count_for_version $v
done
