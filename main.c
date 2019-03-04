/* ember - EMBedding helpER */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>

const char special[] = "{}[]#()%:;.?*+-/^&|~!=,\\\"'";
const char strSpecial[] = "\a\b\f\n\r\v\\\"";
const char strEscape[] = "abfnrv\\\"";

const char usage[] =
"Usage:	ember [-o file] {[input args] files}*\n"
"Ember (EMBedding helpER) turns files into char arrays and packs them into C headers\n"
"Input arguments only affect the files that come after them.\n"
"All arguments are case-insensitive.\n"
"Arguments that have a limited set of allowed values also accept shorthands and ignore case.\n"
"\n"
"-o\n"
"	Sets the output file to use. stdout by default.\n"
"\n"
"Available input arguments are:\n"
"-f\n"
"	Sets the output format. Can either be 'source' or 'header'. Header does not include the actual variable values and marks variables as extern. Overwrites the access modifier. Source by default.\n"
"-p\n"
"	Sets the prefix to use for turning the filename into a variable name. Nothing by default.\n"
"-m\n"
"	Sets the access modifier to use for variables. 'const' by default.\n"
"-e\n"
"	Sets the encoding to expect. Can either be 'binary' or 'ascii'. Determines if the variable value is given in byte or string literals. Binary by default\n"
"-n\n"
"	Sets the name for the next file. Only applies to the next file.\n"
;

static struct
{
	const char *prefix;
	const char *modifier;
	const char *name;
	enum { ENC_BIN, ENC_ASCII } encoding;
	enum { FMT_HEADER, FMT_SOURCE } format;
	FILE *output;
} settings = { "", "const", NULL, ENC_BIN, FMT_SOURCE, NULL };

#define selectOne(s, o) _selectOne(s, o, sizeof(o) / sizeof(*o))
const char *formatOptions[] = { "header", "source" };
const char *encodingOptions[] = { "binary", "ascii" };

bool validName(const char *name)
{
	for(; *name; name++)
	{
		if(isspace(*name) || strchr(special, *name) || iscntrl(*name))
			return false;
	}

	return true;	
}

/* Selects the a value matching str from vals. Allows for shorthands and is case-insensitive.
	Returns -1 if no value is found. */
size_t _selectOne(const char *str, const char **vals, size_t valsCount)
{
	size_t ilast = -1;

	for(size_t i = 0; i < valsCount; i++)
	{
		for(size_t j = 0; str[j]; j++)
			if(tolower(vals[i][j]) != tolower(str[j]))
				goto _continue;
		

		if(ilast != (size_t)-1)
			return -1;

		ilast = i;
		_continue:;
	}

	return ilast;
}

/* Inserts the filename from the given path into settings.output without any reserved C characters.
	Returns false if no chars were written */
bool putFName(const char *path)
{
	char *e = strrchr(path, '/');
	if(e)
		path = e + 1;

	bool any = false;
	bool makeUpper = false;

	for(; *path; path++)
	{
		if(isspace(*path))
		{
			makeUpper = true;
			continue;
		}
		if(*path == '.')
			break;
		if(strchr(special, *path))
			continue;

		fputc(makeUpper ? toupper(*path) : *path, settings.output);
		makeUpper = false;
		any = true;
	}

	return any;
}

void insert(FILE *f)
{
	if(settings.encoding == ENC_ASCII)
	{
		fprintf(settings.output, "\n\"");
		bool needQ = false;

		int c;
		while(c = fgetc(f), c != EOF)
		{
			if(needQ)
			{
				fprintf(settings.output, "\"\n\"");
				needQ = false;
			}

			if(!c)
				fprintf(stderr, "WARNING: Inserting 0-byte into string literal\n");

			char *sp;
			if((sp = strchr(strSpecial, c)))
				fprintf(settings.output, "\\%c", strEscape[sp - strSpecial]);
			else if(c >= 0x80 || (!isprint(c) && !isspace(c)))
				fprintf(settings.output, "\\x%02hhx", c);
			else
				fputc(c, settings.output);

			if(c == '\n')
				needQ = true;
		}

		fputs("\";", settings.output);
	}
	else
	{
		fputc('{', settings.output);

		int c;
		bool first = true;
		while(c = fgetc(f), c != EOF)
		{
			if(!first)
				fputc(',', settings.output);
			
			fprintf(settings.output, " %hhu", c);
			first = false;
		}

		fputs(" };", settings.output);
	}
}

int main(int argc, char **argv)
{
	// skip program name
	argc--;
	argv++;

	if(!argc)
	{
		fputs(usage, stderr);
		return EXIT_SUCCESS;
	}
	if(argv[argc - 1][0] == '-' && argv[argc - 1][1] && !argv[argc - 1][2])
	{
		fprintf(stderr, "Missing value for argument '%s'\n", argv[argc - 1]);
		return EXIT_FAILURE;
	}

	settings.output = stdout;
	#define ABORT(...) { fprintf(stderr, __VA_ARGS__); fclose(settings.output); return EXIT_FAILURE; }

	// the index of the next argument to read
	int i = 0;

	// check for a -o argument
	if(argv[i][0] == '-' && (argv[i][1] == 'o' || argv[i][1] == 'O') && !argv[i][2])
	{
		i++;
		
		if(!(settings.output = fopen(argv[i], "w")))
			ABORT("Cannot open output file '%s'\n", argv[i]);
		
		i++;
	}

	bool gotStdin = false;

	for(; i < argc; i++)
	{
		FILE *f;

		if(argv[i][0] == '-')
		{
			if(!argv[i][1])
			{
				if(gotStdin)
					ABORT("Already read from stdin\n")

				f = stdin;
				gotStdin = true;
				goto got_stdin;		
			}

			if(argv[i][2])
				goto got_filename;

			const char *val = argv[i + 1];

			switch(argv[i][1])
			{
				case 'p':
				case 'P':
					if(!validName(settings.prefix = val))
						ABORT("Invalid prefix '%s'\n", val)
				break;

				case 'm':
				case 'M':
					settings.modifier = val;
				break;

				case 'e':
				case 'E':
				{
					size_t _enc = selectOne(val, encodingOptions);

					if(_enc == (size_t)-1)
						ABORT("Unknown encoding '%s'\n", val)

					settings.encoding = _enc;
				}
				break;

				case 'f':
				case 'F':
					switch(settings.format = selectOne(val, formatOptions))
					{
						case FMT_HEADER:
							settings.modifier = "extern const";
						break;

						case FMT_SOURCE:
							settings.modifier = "const";
						break;

						default:
							ABORT("Unknown format '%s'\n", val)
					}
				break;

				case 'n':
				case 'N':
					if(!validName(settings.name = val))
						ABORT("Invalid name '%s'\n", val)
				break;
					
				default:
					ABORT("Unknown input argument '%s'\n", argv[i])
			}

			i++;
			continue;
		}

		got_filename:
		if(!(f = fopen(argv[i], "rb")))
			ABORT("Cannot open input file '%s'\n", argv[i])
		
		got_stdin:
		#undef ABORT
		#define ABORT(...) { fprintf(stderr, __VA_ARGS__); fclose(settings.output); fclose(f); return EXIT_FAILURE; }

		if(!settings.name && f == stdin)
			ABORT("Stdin needs a set name\n")

		fprintf(settings.output, "%s char %s", settings.modifier, settings.prefix);
		if(settings.name)
		{
			fprintf(settings.output, "%s", settings.name);
			settings.name = NULL;
		}
		else if(!putFName(argv[i]))
			ABORT("Cannot use any characters from filename '%s' as variable name\n", argv[i])

		fprintf(settings.output, "[]");

		if(settings.format == FMT_SOURCE)
		{
			fprintf(settings.output, " = ");
			insert(f);
		}
		else
			fputc(';', settings.output);
		
		fputc('\n', settings.output);
	}

	fflush(settings.output);
	fclose(settings.output);
}