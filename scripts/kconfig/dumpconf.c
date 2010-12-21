/*
 * Copyright (C) 2009 Reinhard Tartler
 * Released under the terms of the GNU GPL v2.0.
 */

#include <locale.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

#define LKC_DIRECT_LINK
#include "lkc.h"


struct choice_list {
	struct choice_list *next;
	struct menu *item;
};


static struct menu *current_choice = 0;
static int choice_count = 0;

extern char *choicestring;

void my_print_symbol(FILE *out, struct menu *menu)
{
	struct symbol *sym = menu->sym;
	struct property *prop;
	static char buf[12];
	tristate is_tristate = no;

	if (sym_is_choice(sym)) {
		fprintf(out, "#startchoice\n");
		current_choice = menu;
		choice_count++;

		fprintf(out, "Choice\tCHOICE_%d", choice_count);
		snprintf(buf, sizeof buf, "CHOICE_%d", choice_count);

		// optional, i.e. all items can be deselected
		if (current_choice->sym->flags & SYMBOL_OPTIONAL)
			fprintf(out, "\toptional");
		else
			fprintf(out, "\trequired");

		if (current_choice->sym->type & S_TRISTATE)
			fprintf(out, "\ttristate");
		else
			fprintf(out, "\tboolean");

		fprintf(out, "\n");

	} else {
		if (current_choice)
			fprintf(out, "ChoiceItem\t%s\t%s\n", sym->name, buf);

		fprintf(out, "Item\t%s", sym->name);
		switch (sym->type) {
		case S_BOOLEAN:
			fputs("\tboolean\n", out);
			is_tristate = yes;
			break;
		case S_TRISTATE:
			fputs("\ttristate\n", out);
			is_tristate = mod;
			break;
		case S_STRING:
			fputs("\tstring\n", out);
			break;
		case S_INT:
			fputs("\tinteger\n", out);
			break;
		case S_HEX:
			fputs("\thex\n", out);
			break;
		default:
			fputs("\t???\n", out);
			break;
		}
	}

	if (menu->dep || is_tristate != no) {
		char itemname[50];

		if (sym->name)
			snprintf(itemname, sizeof itemname, "%s", sym->name);
		else {
			snprintf(itemname, sizeof itemname, "CHOICE_%d", choice_count);
			choicestring = buf;
		}
		if (menu->dep) {
		    fprintf(out, "Depends\t%s\t\"", itemname);
		    expr_fprint(menu->dep, out);
		    fprintf(out, "\"\n");
		}
		
		for_all_properties(sym, prop, P_DEFAULT) {
			fprintf(out, "Default\t%s\t\"", itemname);
			expr_fprint(prop->expr, out);
			fprintf(out, "\"\t\"");
			expr_fprint(prop->visible.expr, out);
			fprintf(out, "\"\n");
		}
		for_all_properties(sym, prop, P_SELECT) {
			fprintf(out, "ItemSelects\t%s\t\"", itemname);
			expr_fprint(prop->expr, out);
			fprintf(out, "\"\t\"");
			expr_fprint(prop->visible.expr, out);
			fprintf(out, "\"\n");
		}
	}

	for (prop = sym->prop; prop; prop = prop->next) {
		if (prop->menu != menu)
			continue;
		switch (prop->type) {
		case P_CHOICE:
			fputs("#choice value\n", out);
			break;
#if 0
                case P_DEFAULT:
                    fprintf(out, "#default\t%s\t%s\t\"", sym->name, prop->text);
                    expr_fprint(prop->visible.expr, out);
                    fprintf(out, "\"\n");
                    break;
                case P_SELECT:
                    fprintf(out, "#select\t%s\t\"", sym->name);                                                  
                    expr_fprint(prop->expr, out);
                    fprintf(out, "\"\t\"");
                    expr_fprint(prop->visible.expr, out);
                    fprintf(out, "\"\n");
                    break;
                case P_PROMPT:
		    fprintf(out, "#prompt\t%s\t", prop->sym->name);
		    expr_fprint(prop->visible.expr, out);
		    fprintf(out, "\n");
		    break;
		default:
			fprintf(out, "  unknown prop %d!\n", prop->type);
			break;
#endif
		}
	}
}

void myconfdump(FILE *out)
{
	struct property *prop;
	struct symbol *sym;
	struct menu *menu;

	menu = rootmenu.list;
	while (menu) {
		if ((sym = menu->sym))
			my_print_symbol(out, menu);
		else if ((prop = menu->prompt)) {
		}

		if (menu->list)
			menu = menu->list;
		else if (menu->next)
			menu = menu->next;
		else while ((menu = menu->parent)) {
			if (current_choice)
				fprintf(out, "#endchoice\n");
			current_choice = 0;
			if (menu->next) {
				menu = menu->next;
				break;
			}
		}
	}
}

int main(int ac, char **av)
{
	int opt;
	struct stat tmpstat;
	char *arch = getenv("ARCH");

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	if (stat(av[1], &tmpstat) != 0) {
		fprintf(stderr, "could not open %s\n", av[1]);
		exit(EXIT_FAILURE);
	}

	if (!arch) {
		fputs("setting arch", stderr);
		arch = strdup ("x86");
	}
	fprintf(stderr, "using arch %s\n", arch);
	setenv("ARCH", arch, 1);
	setenv("KERNELVERSION", "2.6.30-vamos", 1);
	conf_parse(av[1]);
	myconfdump(stdout);
	return 0;
}
