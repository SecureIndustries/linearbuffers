
%{

#include "parser.h"
#include "parser.tab.h"

%}

blanks                  [ \t\n]+
string                  [_.a-zA-Z0-9]+
type                    [_a-zA-Z0-9]+

%x c_comment
%x cpp_comment

%%

{blanks}                { /* ignore */ }

"namespace"             return(NAMESPACE);
"enum"                  return(ENUM);
"table"                 return(TABLE);
"{"                     return(BLOCK);
"}"                     return(ENDBLOCK);
":"                     return(COLON);
";"                     return(SEMICOLON);
","                     return(COMMA);
"="                     return(EQUAL);
"["                     return(VECTOR);
"]"                     return(ENDVECTOR);

{string}                {
                            yylval.sval = strdup(yytext);
                            return(STRING);
                        }

"/*"                    BEGIN(c_comment);

<c_comment>[^*\n]*      /* eat anything that's not a '*' */
<c_comment>"*"+[^*/\n]* /* eat up '*'s not followed by '/'s */
<c_comment>\n           /* eat \n */
<c_comment>"*"+"/"      BEGIN(INITIAL);

"//"                    BEGIN(cpp_comment);

<cpp_comment>[^\n]*     /* eat anything that's not a '*' */
<cpp_comment>\n         BEGIN(INITIAL);
