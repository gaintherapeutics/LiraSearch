#include <list>
#include <string>
#include <fstream>

#include <CGAL/Polyhedron_3.h>
#include <CGAL/Skin_surface_3.h>
#include <CGAL/mesh_skin_surface_3.h>
#include <CGAL/Side_of_triangle_mesh.h>
#include <CGAL/subdivide_skin_surface_mesh_3.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>

#include "include/file_parser.h"
#include "include/skin_surface_writer.h"
#include "include/skin_surface_processor.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Skin_surface_traits_3 <K> Traits;
typedef CGAL::Skin_surface_3 <Traits> Skin_surface_3;
typedef CGAL::Polyhedron_3 <K, CGAL::Skin_surface_polyhedral_items_3<Skin_surface_3>> Polyhedron;

typedef Skin_surface_3::FT FT;
typedef Skin_surface_3::Weighted_point Weighted_point;

typedef K::Point_3 Point_3;


int main(int argc, char *argv[]) {
    // CGAL container for weighted points
    std::list <Weighted_point> weighted_points_3;

    // Read a cif file
    auto file_name = argv[1];
    cif::Document document = gemmi::cif::read_file(file_name);

    // Extract the points from the file
    auto atoms = extract_weighted_points_from_file(weighted_points_3, document);

    // CGAL polyhedron container
    Polyhedron surface_polyhedron;

    // Shrink factor for the skin surface
    FT shrink_factor = 0.5;

    // Generate the skin surface
    Skin_surface_3 skin_surface(weighted_points_3.begin(), weighted_points_3.end(), shrink_factor);

    // Generate the mesh for the skin surface
    CGAL::mesh_skin_surface_3(skin_surface, surface_polyhedron);

    // Subdivide the skin surface's mesh
    CGAL::subdivide_skin_surface_mesh_3(skin_surface, surface_polyhedron);

    // Test if origin is inside the surface
    // (origin is system's origin)
    CGAL::Side_of_triangle_mesh <Polyhedron, K> inside(surface_polyhedron);
    Point_3 origin(0.0, 0.0, 0.0);
    CGAL::Bounded_side res = inside(origin);
    if (res == CGAL::ON_BOUNDED_SIDE) { std::cout << "# origin (" << origin << ") is inside\n"; }

    // Process mesh
    process_polyhedron(surface_polyhedron);

    // Output the mesh
    std::ofstream out(argv[2]);
    write_polyhedron_with_normals(skin_surface, surface_polyhedron, out);
    out.close();

    return 0;
}


