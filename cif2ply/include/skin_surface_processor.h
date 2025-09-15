#ifndef CGAL_SKIN_SURFACE_PROCESSOR_H
#define CGAL_SKIN_SURFACE_PROCESSOR_H

#include <CGAL/Surface_mesh_simplification/edge_collapse.h>
#include <CGAL/Surface_mesh_simplification/Policies/Edge_collapse/Edge_count_stop_predicate.h>

namespace SMS = CGAL::Surface_mesh_simplification;

template<class Polyhedron>
/// Process polyhedron for SH coefficients:
void process_polyhedron(Polyhedron &p) {
    const std::size_t edge_count_threshold = 10000;
    SMS::Edge_count_stop_predicate<Polyhedron> stop(edge_count_threshold);

    int r = SMS::edge_collapse(p, stop,
                               CGAL::parameters::vertex_index_map(get(CGAL::vertex_external_index, p))
                                       .halfedge_index_map(get(CGAL::halfedge_external_index, p)));
    std::cout << "Finished!\n" << r << " edges removed.\n"
              << (p.size_of_halfedges() / 2) << " final edges.\n";

}
#endif // CGAL_SKIN_SURFACE_PROCESSOR_H
