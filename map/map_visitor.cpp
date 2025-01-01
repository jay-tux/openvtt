//
// Created by jay on 12/14/24.
//

#include "map_visitor.hpp"
#include "renderer/log_view.hpp"

using namespace openvtt::map;
using namespace openvtt::renderer;

std::any map_visitor::visitProgram(mapParser::ProgramContext *context) {
  visit_through(context->voxels, at(*context));
  visit_through(context->objects, at(*context));
  return no_value{at(*context)}; // no reasonable return value possible
}

std::any map_visitor::visitVoxelSpec(mapParser::VoxelSpecContext *context) {
  return no_value{at(*context)}; // TODO?
}

std::any map_visitor::visitObjectsSpec(mapParser::ObjectsSpecContext *context) {
  for (const auto stmt: context->body) {
    visit_through(stmt, at(*context));
  }
  return no_value{at(*context)}; // no reasonable return value possible
}

std::any map_visitor::visitLoadObject(mapParser::LoadObjectContext *context) {
  return value{
    visit_maybe_value<std::string>(context->asset, at(*context)) |
      [](const std::string &x){ return render_cache::load<render_object>(x); } ||
      []{ return object_ref::invalid(); },
    at(*context)
  };
}

std::any map_visitor::visitLoadInstancedObject(mapParser::LoadInstancedObjectContext *context) {
  const auto asset = visit_maybe_value<std::string>(context->asset, at(*context));
  const auto transforms = visit_maybe_value<std::vector<value>>(context->ts, at(*context));
  if (asset.has_value() && transforms.has_value()) {
    std::vector<glm::mat4> mats;
    mats.reserve(transforms->size());
    for (const auto &t: *transforms) {
      const auto m = t.should_be<glm::mat4>();
      mats.push_back(m);
    }

    return value{render_cache::load<instanced_object>(*asset, mats), at(*context)};
  }
  return value{instanced_object_ref::invalid(), at(*context)};
}

std::any map_visitor::visitLoadShader(mapParser::LoadShaderContext *context) {
  const auto vs = visit_maybe_value<std::string>(context->vs, at(*context));
  const auto fs = visit_maybe_value<std::string>(context->fs, at(*context));
  return value{vs && fs ? render_cache::load<shader>(*vs, *fs) : shader_ref::invalid(), at(*context)};
}

std::any map_visitor::visitLoadTexture(mapParser::LoadTextureContext *context) {
  return value{
    visit_maybe_value<std::string>(context->asset, at(*context)) |
      [](const std::string &x){ return render_cache::construct<texture>(x); } ||
      []{ return texture_ref::invalid(); },
    at(*context)
  };
}

std::any map_visitor::visitLoadCollider(mapParser::LoadColliderContext *context) {
  return value{
    visit_maybe_value<std::string>(context->asset, at(*context)) |
      [](const std::string &x){ return render_cache::load<collider>(x); } ||
      []{ return collider_ref::invalid(); },
    at(*context)
  };
}

std::any map_visitor::visitLoadInstancedCollider(mapParser::LoadInstancedColliderContext *context) {
  const auto asset = visit_maybe_value<std::string>(context->asset, at(*context));
  const auto transforms = visit_maybe_value<std::vector<value>>(context->ts, at(*context));
  if (asset.has_value() && transforms.has_value()) {
    std::vector<glm::mat4> mats;
    mats.reserve(transforms->size());
    for (const auto &t: *transforms) {
      const auto m = t.should_be<glm::mat4>();
      mats.push_back(m);
    }

    return value{render_cache::load<instanced_collider>(*asset, mats), at(*context)};
  }
  return value{instanced_collider_ref::invalid(), at(*context)};
}

std::any map_visitor::visitLoadTransform(mapParser::LoadTransformContext *context) {
  const auto pos = visit_maybe_value<glm::vec3>(context->p, at(*context));
  const auto rot = visit_maybe_value<glm::vec3>(context->r, at(*context));
  const auto scale = visit_maybe_value<glm::vec3>(context->s, at(*context));

  if (pos.has_value() && rot.has_value() && scale.has_value()) {
    return value{instanced_object::model_for(*rot, *scale, *pos), at(*context)};
  }
  return value{glm::mat4{1.0f}, at(*context)};
}

std::any map_visitor::visitExprList(mapParser::ExprListContext *context) {
  std::vector<value> values;
  values.reserve(context->exprs.size());
  for (const auto &expr : context->exprs) {
    visit_maybe_value(expr, at(*context)) | [&values](const value &v) { values.push_back(v); };
  }
  return values;
}

std::any map_visitor::visitIdExpr(mapParser::IdExprContext *context) {
  return identifier{context->x->getText()};
}

std::any map_visitor::visitIntExpr(mapParser::IntExprContext *context) {
  return value{std::stoi(context->x->getText()), at(*context)};
}

std::any map_visitor::visitFloatExpr(mapParser::FloatExprContext *context) {
  return value{std::stof(context->x->getText()), at(*context)};
}

std::any map_visitor::visitStringExpr(mapParser::StringExprContext *context) {
  const std::string x = context->x->getText();
  return value{x.substr(1, x.size() - 2), at(*context)};
}

std::any map_visitor::visitTupleExpr(mapParser::TupleExprContext *context) {
  const auto x = visit_maybe_value(context->x, at(*context));
  const auto y = visit_maybe_value(context->y, at(*context));
  if (x.has_value() && y.has_value()) {
    return value{value_pair{*x, *y}, at(*context)};
  }
  return value{value_pair{}, at(*context)};
}

std::any map_visitor::visitVec3Expr(mapParser::Vec3ExprContext *context) {
  const auto x = visit_expect<float>(context->x, at(*context));
  const auto y = visit_expect<float>(context->y, at(*context));
  const auto z = visit_expect<float>(context->z, at(*context));
  return value{glm::vec3{x, y, z}, at(*context)};
}

std::any map_visitor::visitEmptyListExpr(mapParser::EmptyListExprContext *context) {
  return value{std::vector<value>{}, at(*context)};
}

std::any map_visitor::visitListExpr(mapParser::ListExprContext *context) {
  return value{visit_no_value<std::vector<value>>(context->exprs, at(*context), "value list"), at(*context)};
}

std::any map_visitor::visitLoadExpr(mapParser::LoadExprContext *context) {
  return visit_through(context->ld, at(*context));
}

std::any map_visitor::visitAssignExpr(mapParser::AssignExprContext *context) {
  return visit_maybe_value(context->value, at(*context)) | [context, this](value v) {
    cache.assign(context->x->getText(), std::move(v), at(*context));
    return std::any{identifier{context->x->getText()}};
  } || [this, context]{ return no_value{at(*context)}; };
}

std::any map_visitor::visitSpawnExpr(mapParser::SpawnExprContext *context) {
  const auto args_pre = expect_n_args(context->args, at(*context), 4);
  if (!args_pre.has_value()) return render_ref::invalid();
  const auto &args = *args_pre;

  const auto name = args[0].should_be<std::string>();
  const auto obj = args[1].should_be<object_ref>();
  const auto sh = args[2].should_be<shader_ref>();
  std::vector<std::pair<unsigned int, texture_ref>> textures;
  for (const auto tex_pre = args[3].should_be<std::vector<value>>(); const auto &t: tex_pre) {
    const auto p = t.should_be<value_pair>();
    const auto [idx, tex] = p.as_pair(); // do not merge with p ~> otherwise .should_be result gets destroyed
    const auto idx_fix = idx.should_be<int>();
    const auto tex_fix = tex.should_be<texture_ref>();
    textures.emplace_back(idx_fix, tex_fix);
  }

  const auto ref = render_cache::construct<renderable>(name, obj, sh, uniforms::from_shader(sh), textures);

  spawned.insert(ref);
  return value{ref, at(*context)};
}

std::any map_visitor::visitInstancedSpawnExpr(mapParser::InstancedSpawnExprContext *context) {
  const auto args_pre = expect_n_args(context->args, at(*context), 4);
  if (!args_pre.has_value()) return render_ref::invalid();
  const auto &args = *args_pre;

  const auto name = args[0].should_be<std::string>();
  const auto obj = args[1].should_be<instanced_object_ref>();
  const auto sh = args[2].should_be<shader_ref>();
  std::vector<std::pair<unsigned int, texture_ref>> textures;
  for (const auto tex_pre = args[3].should_be<std::vector<value>>(); const auto &t: tex_pre) {
    const auto p = t.should_be<value_pair>();
    const auto [idx, tex] = p.as_pair(); // do not merge with p ~> otherwise .should_be result gets destroyed
    const auto idx_fix = idx.should_be<int>();
    const auto tex_fix = tex.should_be<texture_ref>();
    textures.emplace_back(idx_fix, tex_fix);
  }

  const auto ref = render_cache::construct<instanced_renderable>(name, obj, sh, instanced_uniforms::from_shader(sh), textures);

  spawned_instances.insert(ref);
  return value{ref, at(*context)};
}

std::any map_visitor::visitTransformExpr(mapParser::TransformExprContext *context) {
  const auto args = expect_n_args(context->args, at(*context), 4);
  if (!args.has_value()) return no_value{at(*context)}; // no reasonable return value possible

  const auto ref = (*args)[0].should_be<render_ref>();
  const auto pos = (*args)[1].should_be<glm::vec3>();
  const auto rot = (*args)[2].should_be<glm::vec3>();
  const auto scale = (*args)[3].should_be<glm::vec3>();

  ref->position = pos; ref->rotation = rot; ref->scale = scale;
  return no_value{at(*context)}; // no reasonable return value possible
}

std::any map_visitor::visitExprStmt(mapParser::ExprStmtContext *context) {
  visit_through(context->e, at(*context));
  return no_value{at(*context)}; // no reasonable return value possible
}

std::any map_visitor::visitEnableHighlightStmt(mapParser::EnableHighlightStmtContext *context) {
  visit_maybe_value<shader_ref>(context->x, at(*context)) | [this, context](const shader_ref sh) {
    visit_maybe_value<std::string>(context->uniform, at(*context)) | [this, context, sh](const std::string &uniform) {
      visit_maybe_value<std::string>(context->toggle, at(*context)) | [this, sh, uniform](const std::string &toggle) {
        log<log_type::DEBUG>("map_visitor", std::format("Shader ready for highlighting; uniform: {}, toggle: {}", uniform, toggle));
        requires_highlight[sh] = {
          .uniform_tex = sh->loc_for(uniform),
          .uniform_highlight = sh->loc_for(toggle)
        };
      };
    };
  };

  return no_value{at(*context)}; // no reasonable return value possible
}

std::any map_visitor::visitEnableInstancedHighlightStmt(mapParser::EnableInstancedHighlightStmtContext *context) {
  visit_maybe_value<shader_ref>(context->x, at(*context)) | [this, context](const shader_ref sh) {
    visit_maybe_value<std::string>(context->uniform, at(*context)) | [this, context, sh](const std::string &uniform) {
      visit_maybe_value<std::string>(context->toggle, at(*context)) | [this, context, sh, uniform](const std::string &toggle) {
        visit_maybe_value<std::string>(context->inst, at(*context)) | [this, sh, uniform, toggle](const std::string &inst) {
          log<log_type::DEBUG>("map_visitor", std::format("Shader ready for highlighting; uniform: {}, toggle: {}, inst: {}", uniform, toggle, inst));
          requires_instanced_highlight[sh] = {
            .uniform_tex = sh->loc_for(uniform),
            .uniform_highlight = sh->loc_for(toggle),
            .uniform_instance_id = sh->loc_for(inst)
          };
        };
      };
    };
  };

  return no_value{at(*context)}; // no reasonable return value possible
}

std::any map_visitor::visitHighlightBindStmt(mapParser::HighlightBindStmtContext *context) {
  visit_maybe_value<int>(context->x, at(*context)) | [this](const int idx) {
    highlight_binding = idx;
  };

  return no_value{at(*context)}; // no reasonable return value possible
}

std::any map_visitor::visitAddColliderStmt(mapParser::AddColliderStmtContext *context) {
  visit_maybe_value<render_ref>(context->x, at(*context)) | [this, context](const render_ref ref) {
    visit_maybe_value<collider_ref>(context->coll, at(*context)) | [ref](const collider_ref coll) {
      ref->coll = coll;
    };
  };

  return no_value{at(*context)}; // no reasonable return value possible
}

std::any map_visitor::visitAddInstancedColliderStmt(mapParser::AddInstancedColliderStmtContext *context) {
  visit_maybe_value<instanced_render_ref>(context->x, at(*context)) | [this, context](const instanced_render_ref ref) {
    visit_maybe_value<instanced_collider_ref>(context->coll, at(*context)) | [ref](const instanced_collider_ref coll) {
      ref->coll = coll;
    };
  };

  return no_value{at(*context)}; // no reasonable return value possible
}