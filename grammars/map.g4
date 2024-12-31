grammar map;

WS: [ \t\r\n]+ -> skip;
STRING: '"' (~["\r\n\\])* '"' /*{
    setText(getText().substr(1, getText().length() - 2));
}*/;
IDENTIFIER: [a-zA-Z_][a-zA-Z0-9_]*;
INT: [+-]?[0-9]+;
FLOAT: [+-]?([0-9]*[.])?[0-9]+;
COMMENT: '//' ~[\r\n]* -> skip;

program: voxels=voxelSpec objects=objectsSpec | objects=objectsSpec voxels=voxelSpec;

voxelSpec: 'voxels' '(' w=INT ',' h=INT ')' '{' /*TODO*/ '}';

objectsSpec: 'objects' '{' body+=stmt* '}';

load: 'object' '(' asset=expr ')'                 #loadObject
    | 'shader' '(' vs=expr ',' fs=expr ')'        #loadShader
    | 'texture' '(' asset=expr ')'                #loadTexture
    | 'collider' '(' asset=expr ')'               #loadCollider
//    | 'font' '(' asset=expr ')'                   #loadFont
    ;

exprList: exprs+=expr (',' exprs+=expr)*;

expr: x=IDENTIFIER                                  #idExpr
    | x=INT                                         #intExpr
    | x=FLOAT                                       #floatExpr
    | x=STRING                                      #stringExpr
    | '(' x=expr ',' y=expr ')'                     #tupleExpr
    | '(' x=expr ',' y=expr ',' z=expr ')'          #vec3Expr
    | '[' exprs=exprList ']'                        #listExpr
    | ld=load                                       #loadExpr
    | x=IDENTIFIER '=' value=expr                   #assignExpr
    | 'spawn' '(' args=exprList ')'                 #spawnExpr
    | 'transform' '(' args=exprList ')'             #transformExpr
    | 'add_collider' '(' args=exprList ')'          #addColliderExpr
    ;

stmt: e=expr ';'                                    #exprStmt
    | 'enable_highlight' '(' x=expr ',' uniform=expr ')' ';'
                                                    #enableHighlightStmt
    | 'highlight_bind' '(' x=expr ')' ';'           #highlightBindStmt
    ;

/*
 -- Value types: --
  -> int
  -> float
  -> string (std::string)
  -> vec3 (glm::vec3)
  -> list (std::vector; strongly typed, of any type)

  -> object (renderer::object_ref)
  -> instanced_object (renderer::instanced_object_ref) (TODO: grammar)
  -> shader (renderer::shader_ref)
  -> texture (renderer::texture_ref)
  -> collider (renderer::collider_ref)
  -> instanced_collider (renderer::instanced_collider_ref) (TODO: grammar)
  -> font (TODO)
  -> renderable (renderer::render_ref)
  -> instanced_renderable (renderer::instanced_render_ref) (TODO: grammar)
  -> transform (TODO)
*/