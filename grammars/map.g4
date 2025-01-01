grammar map;

WS: [ \t\r\n]+ -> skip;
STRING: '"' (~["\r\n\\])* '"' /*{
    setText(getText().substr(1, getText().length() - 2));
}*/;
IDENTIFIER: [a-zA-Z_][a-zA-Z0-9_]*;
FUNC_NAME: '@'[a-zA-Z_][a-zA-Z0-9_*]*;
INT: [+-]?[0-9]+;
FLOAT: [+-]?([0-9]*[.])?[0-9]+;
COMMENT: '//' ~[\r\n]* -> skip;

program: voxels=voxelSpec objects=objectsSpec | objects=objectsSpec voxels=voxelSpec;

voxelSpec: 'voxels' '(' w=INT ',' h=INT ')' '{' /*body+=voxelStmt**/ '}';

objectsSpec: 'objects' '{' body+=objStmt* '}';

//load: 'object' '(' asset=expr ')'                   #loadObject
//    | 'object' '*' '(' asset=expr ',' ts=expr ')'   #loadInstancedObject
//    | 'shader' '(' vs=expr ',' fs=expr ')'          #loadShader
//    | 'texture' '(' asset=expr ')'                  #loadTexture
//    | 'collider' '(' asset=expr ')'                 #loadCollider
//    | 'collider' '*' '(' asset=expr ',' ts=expr ')' #loadInstancedCollider
//    | 'transform' '(' p=expr ',' r=expr ',' s=expr ')'
//                                                    #loadTransform
//    | 'font' '(' asset=expr ')'                   #loadFont
//    ;

exprList: exprs+=expr (',' exprs+=expr)*;

expr: x=IDENTIFIER                                  #idExpr
    | x=INT                                         #intExpr
    | x=FLOAT                                       #floatExpr
    | x=STRING                                      #stringExpr
    | '(' x=expr ',' y=expr ')'                     #tupleExpr
    | '(' x=expr ',' y=expr ',' z=expr ')'          #vec3Expr
    | '[' ']'                                       #emptyListExpr
    | '[' exprs=exprList ']'                        #listExpr
//    | ld=load                                       #loadExpr
    | x=IDENTIFIER '=' value=expr                   #assignExpr
    | x=FUNC_NAME '(' args=exprList ')'             #funcExpr
//    | 'spawn' '(' args=exprList ')'                 #spawnExpr
//    | 'spawn' '*' '(' args=exprList ')'             #instancedSpawnExpr
    ;

objStmt: e=expr ';'                                 #exprStmt
//    | 'transform_obj' '(' args=exprList ')'         #transformStmt
//    | 'enable_highlight' '(' x=expr ',' uniform=expr ',' toggle=expr ')' ';'
//                                                    #enableHighlightStmt
//    | 'enable_highlight' '*' '(' x=expr ',' uniform=expr ',' toggle=expr ',' inst=expr ')' ';'
//                                                    #enableInstancedHighlightStmt
//    | 'highlight_bind' '(' x=expr ')' ';'           #highlightBindStmt
//    | 'add_collider' '(' x=expr ',' coll=expr ')' ';'
//                                                    #addColliderStmt
//    | 'add_collider' '*' '(' x=expr ',' coll=expr ')' ';'
//                                                    #addInstancedColliderStmt
    ;

//voxelStmt: e=expr ';'                               #vExprStmt
//    |
//    ;

/*
 -- Value types: --
  -> int
  -> float
  -> string (std::string)
  -> vec3 (glm::vec3)
  -> list (std::vector; strongly typed, of any type)

  -> object (renderer::object_ref)
  -> instanced_object (renderer::instanced_object_ref)
  -> shader (renderer::shader_ref)
  -> texture (renderer::texture_ref)
  -> collider (renderer::collider_ref)
  -> instanced_collider (renderer::instanced_collider_ref)
  -> font (TODO)
  -> renderable (renderer::render_ref)
  -> instanced_renderable (renderer::instanced_render_ref)
  -> transform
*/