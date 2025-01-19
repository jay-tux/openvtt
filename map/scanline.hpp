//
// Created by jay on 1/18/25.
//

#ifndef SCANLINE_HPP
#define SCANLINE_HPP

#include <ranges>
#include <vector>
#include <unordered_set>
#include <generator>

namespace openvtt::map {
/**
 * @brief Shorthand for `static_cast<int>(std::round(x))`.
 * @param x The value to round.
 * @return The rounded value (as int).
 */
constexpr int round_to_int(const float x) {
  return static_cast<int>(std::round(x));
}

/**
 * @brief Gets the x-coordinate of the intersection of a horizontal line with a line segment (that isn't horizontal).
 * @param y_hor The y-coordinate of the horizontal line.
 * @param p1 The first point of the line segment.
 * @param p2 The second point of the line segment.
 * @return The x-coordinate of the intersection (even if it's outside the segment).
 */
constexpr float intersection_x(const int y_hor, const std::pair<int, int> &p1, const std::pair<int, int> &p2) {
  const auto &[x1, y1] = p1;
  const auto &[x2, y2] = p2;
  return static_cast<float>(x1) + static_cast<float>(y_hor - y1) * static_cast<float>(x2 - x1) / static_cast<float>(y2 - y1);
}

/**
 * @brief Checks if an integer is between two other integers.
 * @param x The integer to check.
 * @param a The first bound.
 * @param b The second bound.
 * @return True if `a <= x <= b` or `b <= x <= a`, false otherwise.
 *
 * The order of `a` and `b` doesn't matter, and they are inclusive.
 */
constexpr bool between(const int x, const int a, const int b) {
  return (x >= a && x <= b) || (x >= b && x <= a);
}

/**
 * @brief Checks if a float is between two integers.
 * @param x The float to check.
 * @param a The first bound.
 * @param b The second bound.
 * @return True if `a <= x <= b` or `b <= x <= a`, false otherwise.
 *
 * The order of `a` and `b` doesn't matter, and they are inclusive.
 * Both `a` and `b` are cast to float before comparison.
 */
constexpr bool between(const float x, const int a, const int b) {
  return (x >= static_cast<float>(a) && x <= static_cast<float>(b)) || (x >= static_cast<float>(b) && x <= static_cast<float>(a));
}

/**
 * @brief Performs scanline filling on a polygon.
 * @param points The points of the polygon.
 * @return The interior (integer) points of the polygon.
 *
 * "Left" points are considered inclusive, while "right" points are exclusive.
 * Horizontal edges are ignored.
 * Passing a polygon where edges intersect each other is undefined behavior. Edges should be broken up into
 * non-intersecting segments.
 */
inline std::vector<std::pair<int, int>> scanline_fill(const std::vector<std::pair<int, int>> &points) {
  using point = std::pair<int, int>;
  using edge = std::pair<point, point>;
  // Create edges
  int y_min = std::numeric_limits<int>::max();
  int y_max = std::numeric_limits<int>::min();

  using namespace renderer;
  log<log_type::DEBUG>("scanline", std::format("Scanline started. Got {} points.", points.size()));

  std::vector<edge> edges;
  edges.reserve(points.size());
  for (size_t i = 0; i < points.size(); i++) {
    const auto &[x1, y1] = points[i];
    const auto &[x2, y2] = points[(i + 1) % points.size()];
    log<log_type::DEBUG>("scanline", std::format("  Checking edge ({}, {}) -> ({}, {}).", x1, y1, x2, y2));

    // ignore horizontal edges
    if (y1 > y2) edges.emplace_back(point{x1, y1}, point{x2, y2});
    else if (y2 > y1) edges.emplace_back(point{x2, y2}, point{x1, y1});

    if (y1 < y_min) y_min = y1;
    if (y2 < y_min) y_min = y2;
    if (y1 > y_max) y_max = y1;
    if (y2 > y_max) y_max = y2;
  }
  log<log_type::DEBUG>("scanline", std::format("Got {} edges (ignored {} horizontal edges).", edges.size(), points.size() - edges.size()));
  log<log_type::DEBUG>("scanline", std::format("Ranging from y={} to y={}.", y_min, y_max));

  std::vector<point> result;
  // Scan lines
  for (int y = y_min; y <= y_max; y++) {
    // Find intersections
    std::vector<std::pair<float, edge>> intersections;
    for (const auto &[p1, p2] : edges) {
      const auto &[x1, y1] = p1;
      const auto &[x2, y2] = p2;
      if (!between(y, y1, y2)) continue; // edge can't intersect

      if (const auto x_int = intersection_x(y, p1, p2); between(x_int, x1, x2)) intersections.emplace_back(x_int, edge{p1, p2});
    }

    // Sort by x
    std::ranges::sort(intersections, [](const auto &p1, const auto &p2) {
      return p1.first < p2.first;
    });

    log<log_type::DEBUG>("scanline", std::format("  -> For y={}, have {} intersections", y, intersections.size()));
    for (const auto &x : intersections | std::ranges::views::keys) {
      log<log_type::DEBUG>("scanline", std::format("    -> Intersection at x={}.", x));
    }
    log<log_type::DEBUG>("scanline", std::format("  -> Filling between intersections."));

    bool filling = true;
    for (size_t i = 0; i < intersections.size() - 1; i++) {
      const auto &[x1, edge1] = intersections[i];
      const auto &[x2, edge2] = intersections[i + 1];
      log<log_type::DEBUG>("scanline", std::format("    -> Piece {} to {}, filling? {}.", x1, x2, filling));

      if (x2 - x1 <= 1e-6f) {
        // both are the same point
        // if bottom of edges, count as interior
        // otherwise, ignore
        // in either case: don't swap flag
        const auto &[xa, ya] = edge1.first;
        const auto &[xb, yb] = edge1.second;
        const auto &[xc, yc] = edge2.first;
        const auto &[xd, yd] = edge2.second;
        if (std::min({ya, yb, yc, yd, y}) == y) {
          log<log_type::DEBUG>("scanline", std::format("      -> Adding single point ({}, {})", round_to_int(x1), y));
          result.emplace_back(round_to_int(x1), y);
        }
      }
      else if (filling) {
        // check if is integer
        //    -> begin: interior if integer
        const int xb = static_cast<int>(std::abs(std::round(x1) - x1) < 1e-6f ? std::floor(x1) : std::ceil(x1));
        //    -> end: exterior if integer
        const int xe = static_cast<int>(std::abs(std::round(x2) - x2) < 1e-6f ? std::floor(x2 - 1.0f) : std::floor(x2));
        log<log_type::DEBUG>("scanline", std::format("      -> Adding piece from {} to {}.", xb, xe));

        for (int x = xb; x <= xe; x++) {
          result.emplace_back(x, y);
          log<log_type::DEBUG>("scanline", "        -> Adding ({}, {}).", x, y);
        }

        // crossed an edge, stop filling
        filling = false;
      }
      else {
        // crossed an edge, start filling again
        filling = true;
      }
    }
  }

  log<log_type::DEBUG>("scanline", std::format("Final results: ({} points)", result.size()));
  for (const auto &[x, y] : result) {
    log<log_type::DEBUG>("scanline", std::format("  -> ({}, {})", x, y));
  }

  return result;
}

/**
 * @brief Determines the border of a region.
 * @param selected The selected points (the region).
 * @param width The width of the border.
 * @return A set of coordinates representing the border.
 */
inline std::vector<std::pair<int, int>> border(const std::vector<std::pair<int, int>> &selected, const int width) {
  const auto extend = [&width](const std::pair<int, int> &point) -> std::generator<std::pair<int, int>> {
    const auto [x0, y0] = point;
    const int xm = x0 - width, xM = x0 + width;
    const int ym = y0 - width, yM = y0 + width;
    for (int x = xm; x <= xM; x++) {
      for (int y = ym; y <= yM; y++) {
        const float dist = std::sqrt(static_cast<float>((x - x0) * (x - x0) + (y - y0) * (y - y0)));
        if (dist <= static_cast<float>(width)) co_yield std::pair{x, y};
      }
    }
  };

  const std::unordered_set<std::pair<int, int>> original{selected.begin(), selected.end()};
  std::unordered_set<std::pair<int, int>> result;
  for (const auto &point: selected) {
    for (const auto &ext: extend(point)) {
      if (!original.contains(ext))
        result.insert(ext);
    }
  }

  return std::vector<std::pair<int, int>>{result.begin(), result.end()};
}
}

#endif //SCANLINE_HPP
