#include "BddLoader.h"
#include "BddContainer.h"

#include <sstream>
#include <iomanip>
#include <fstream>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <sys/stat.h>


const char *BddLoader::CACHE_DIR = ".bddcache";

BddLoader::BddLoader(std::string path)
    : _path(path) {
}

std::string getMd5String(std::string path) {
    BIO *bio, *mdtmp;
    char buf[1024];
    unsigned char mdbuf[16];
    int rdlen;
    std::stringstream ss("");

    bio = BIO_new_file(path.c_str(), "r");
    if(!bio) {
    perror("BIO_new_file");
    return "";
    }

    mdtmp = BIO_new(BIO_f_md());
    BIO_set_md(mdtmp, EVP_md5());
    bio = BIO_push(mdtmp, bio);

    do {
    rdlen = BIO_read(mdtmp, buf, sizeof(buf));
    } while (rdlen > 0);

    int mdlen = BIO_gets(mdtmp, (char*)mdbuf, sizeof(mdbuf));
    for (int i = 0; i < mdlen; i++)
    ss << std::setw(2) << std::setfill('0')
       << std::hex << (unsigned int) mdbuf[i];
    return (ss.str());
}

int BddLoader::makeBdd(bdd &b) {
    struct stat statbuf;
    int error;

    if (0 != stat(CACHE_DIR, &statbuf)) {
    if(0 != mkdir(CACHE_DIR, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
        perror("create cachedir");
        return -1;
    }
    }

    std::ifstream f(_path.c_str());
    if(!f.good()){
    std::clog << "couldn't open: " << _path << std::endl;
    return -1;
    }

    std::string cachedfile = CACHE_DIR + std::string("/")
    + getMd5String(_path) + std::string(".bdd");

    if(0 == stat(cachedfile.c_str(), &statbuf)) {
    if (0 != (error = bdd_fnload((char*)cachedfile.c_str(), b))) {
        std::cerr << "couldn't open bdd file, error code: "
              << error << std::endl;
    }
    return 0;
    }

    BddContainer c(f);
    bdd ret = c.do_build();
    if (0 != (error = bdd_fnsave((char*)cachedfile.c_str(), ret)))
    std::cerr << "couldn't write bdd file, error code: "
          << error << std::endl;
    b = ret;
    return 0;
}

int main (int argc, char **argv) {
    BddLoader l(argv[1]);
    bdd b;

    l.makeBdd(b);
}
