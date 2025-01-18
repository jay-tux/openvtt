grammar map;

WS: [ \t\r\n]+ -> skip;
STRING: '"' (~["\r\n\\])* '"';
IDENTIFIER: [a-zA-Z_][a-zA-Z0-9_]*;
FUNC_NAME: '@'[a-zA-Z_][a-zA-Z0-9_*]*;
INT: [+-]?[0-9]+;
FLOAT: [+-]?([0-9]*[.])?[0-9]+;
COMMENT: '//' ~[\r\n]* -> skip;

program: voxels=voxelSpec objects=objectsSpec | objects=objectsSpec voxels=voxelSpec;

voxelSpec: 'voxels' '(' scale=(INT|FLOAT) ')' '{' body+=voxelBlock* '}';

objectsSpec: 'objects' '{' body+=objStmt* '}';

exprList: exprs+=expr (',' exprs+=expr)*;

expr: x=IDENTIFIER                                  #idExpr
    | 'true'                                        #trueExpr
    | 'false'                                       #falseExpr
    | x=INT                                         #intExpr
    | x=FLOAT                                       #floatExpr
    | x=STRING                                      #stringExpr
    | '(' x=expr ',' y=expr ')'                     #tupleExpr
    | '(' x=expr ',' y=expr ',' z=expr ')'          #vec3Expr
    | '[' ']'                                       #emptyListExpr
    | '[' exprs=exprList ']'                        #listExpr
    | x=IDENTIFIER '=' value=expr                   #assignExpr
    | x=FUNC_NAME '(' args=exprList ')'             #funcExpr
    ;

objStmt: e=expr ';'                                 #exprStmt
    ;

voxelStmt: e=expr ';'                               #vExprStmt
    ;

voxelBlock: s=voxelStmt                             #stmtBlock
    | 'region' '(' r=expr ')' '{' body+=voxelStmt* '}'
                                                    #regionBlock
    ;