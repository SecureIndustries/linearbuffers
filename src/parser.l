
%{

#include "parser.h"
#include "parser.tab.h"

%}

blanks          [ \t\n]+
string          [_.a-zA-Z0-9]+
type            [_a-zA-Z0-9]+

%%

{blanks}        { /* ignore */ }

"namespace"     return(NAMESPACE);
"enum"          return(ENUM);
"table"         return(TABLE);
"root"          return(ROOT);
"{"             return(BLOCK);
"}"             return(ENDBLOCK);
":"             return(COLON);
";"             return(SEMICOLON);
","             return(COMMA);
"="             return(EQUAL);
"["             return(VECTOR);
"]"             return(ENDVECTOR);

{string}        {
                    yylval.sval = strdup(yytext);
                    return(STRING);
                }
