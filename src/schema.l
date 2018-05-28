%{
#include "schema.tab.h"
%}

blanks          [ \t\n]+
string			[_a-zA-Z0-9]+
type			[_a-zA-Z0-9]+

%%

{blanks}        { /* ignore */ }

"enum"			return(ENUM);
"table"			return(TABLE);
"{"				return(BLOCK);
"}"				return(ENDBLOCK);
":"				return(COLON);
";"				return(SEMICOLON);
","				return(COMMA);
"="				return(EQUAL);
"["				return(VECTOR);
"]"				return(ENDVECTOR);

{string}		{
				yylval.sval = strdup(yytext);
				return(STRING);
}
