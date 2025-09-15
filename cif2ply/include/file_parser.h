//
// Created by rinaldo on 4/8/21.
//

#ifndef LIRA_SURFACE_FILE_PARSER_H
#define LIRA_SURFACE_FILE_PARSER_H

#include <list>
#include <limits>
#include <string>

#include <gemmi/cif.hpp>
#include <gemmi/elem.hpp>
#include <gemmi/smcif.hpp>

#include <CGAL/Skin_surface_3.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>


namespace cif = gemmi::cif;

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Skin_surface_traits_3 <K> Traits;
typedef CGAL::Skin_surface_3 <Traits> Skin_surface_3;

typedef Skin_surface_3::FT FT;
typedef Skin_surface_3::Bare_point Bare_point;
typedef Skin_surface_3::Weighted_point Weighted_point;

// Container for the
// parsed data
typedef struct Atom {
    std::string name;
    double x, y, z;
    double vdw_r;
} Atom;


inline std::list <Atom> extract_weighted_points_from_file(std::list <Weighted_point> &weighted_points_3, cif::Document &doc) {
    std::list <Atom> atoms;

    // Check if the cif file was converted from a PDB file
    // Query for the PDB coordinates
    cif::Table selected_atoms = doc.sole_block().find("_atom_site.",
                                                      {"type_symbol", "Cartn_x", "Cartn_y", "Cartn_z"});

    // If the query is positive access the coordinates
    if (selected_atoms.length() > 0) {
        // Loop trough the atoms
        for (auto row : selected_atoms) {
            // Create the current element
            gemmi::Element element(row[0]);

            Atom atom{element.name(),
                      std::stof(row[1]),
                      std::stof(row[2]),
                      std::stof(row[3]),
                      element.vdw_r()};
            // Push the atom to the container
            atoms.push_front(atom);

        }
    } else {
        // If the query fails access data from a
        // small Molecule File Format
        gemmi::SmallStructure SiC = gemmi::make_small_structure_from_block(doc.sole_block());

        // Loop trough the atoms
        for (auto site : SiC.sites) {
            Atom atom{site.element.name(),
                      site.fract.x * SiC.cell.a,
                      site.fract.y * SiC.cell.b,
                      site.fract.z * SiC.cell.c,
                      site.element.vdw_r()};

            // Push the atom to the container
            atoms.push_front(atom);
        }
    }

    // Find the molecule centroid
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    for (auto atom: atoms) {
        x += atom.x;
        y += atom.y;
        z += atom.z;
    }
    x /= atoms.size();
    y /= atoms.size();
    z /= atoms.size();

    std::cout << "# centroid = " << x << " " << y << " " << z << std::endl;

    // Centralize molecule
    for (auto atom: atoms) {
        atom.x -= x;
        atom.y -= y;
        atom.z -= z;

        std::cout << atom.name << ", "
                  << atom.x << ", "
                  << atom.y << ", "
                  << atom.z << ", "
                  << atom.vdw_r << std::endl;

        weighted_points_3.push_front(Weighted_point(
                Bare_point(
                        atom.x,
                        atom.y,
                        atom.z),
                atom.vdw_r));
    }

    return atoms;
}

#endif //LIRA_SURFACE_FILE_PARSER_H
