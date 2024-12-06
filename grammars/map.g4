grammar map;

WS: [ \t\r\n]+ -> skip;
STRING: '"' (~["\r\n\\])* '"';
IDENTIFIER: [a-zA-Z_][a-zA-Z0-9_]*;
INT: [1-9][0-9]*|'0';
COMMENT: '//' ~[\r\n]* -> skip;

program: voxels=voxelSpec objects=objectsSpec | objects=objectsSpec voxels=voxelSpec;

voxelSpec: 'voxels' '(' w=INT ',' h=INT ')' '{' /*TODO*/ '}';

objectsSpec: 'objects' '{' body+=stmt* '}';

load: 'object' '(' asset=STRING ')'                 #loadObject
    | 'shader' '(' vs=STRING ',' fs=STRING ')'      #loadShader
    | 'texture' '(' asset=STRING ')'                #loadTexture
    | 'collider' '(' asset=STRING ')'               #loadCollider
    | 'font' '(' asset=STRING ')'                   #loadFont
    ;

exprList: exprs+=expr (',' exprs+=expr)*;

expr: x=IDENTIFIER                                  #idExpr
    | ld=load                                       #loadExpr
    | x=IDENTIFIER '=' value=expr                   #assignExpr
    | 'combine' '(' args=exprList ')'               #combineExpr
    ;

stmt: e=expr ';'                                    #exprStmt
    ;