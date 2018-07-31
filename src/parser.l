
%{

#include "parser.h"
#include "parser.tab.h"

%}

blanks                  [ \t\n]+
string                  [-_.a-zA-Z0-9]+
text                    \"[^"\n]*["\n]

%x c_comment
%x cpp_comment

%%

{text}                  {
                            yylval.sval = strdup(yytext+1);
                            if (yylval.sval[yyleng-2] != '"') {
                                fprintf(stderr, "improperly terminated string\n");
                            } else {
                                yylval.sval[yyleng-2] = 0;
                                fprintf(stderr, "found '%s'\n", yylval.sval);
                                return(STRING);
                            }
                        }

{blanks}                { /* ignore */ }

"option"                return(OPTION);
"enum"                  return(ENUM);
"table"                 return(TABLE);
"struct"                return(STRUCT);
"{"                     return(BLOCK);
"}"                     return(ENDBLOCK);
":"                     return(COLON);
";"                     return(SEMICOLON);
","                     return(COMMA);
"="                     return(EQUAL);
"["                     return(BRACKET);
"]"                     return(ENDBRACKET);
"("                     return(PARENTHESIS);
")"                     return(ENDPARENTHESIS);

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
