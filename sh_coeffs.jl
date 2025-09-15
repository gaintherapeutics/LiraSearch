#!/usr/bin/env -S julia --color=yes --startup-file=no

using Glob
using Printf
using ArgParse
using TimerOutputs
using Base.Threads
using GeometryBasics

using ImplicitBVH
using ImplicitBVH: BBox, BSphere

include("Geotools.jl")


const to = TimerOutput()

function parse_commandline()
    s = ArgParseSettings()

    @add_arg_table s begin
        "dir"
        help = "The directory with the .tmesh files"
        required = true
        "--lmax", "-l"
        help = "Lmax value"
        arg_type = Int
        default = 16
        "--samples", "-s"
        help = "Number of sampling points"
        arg_type = Int
        default = 20_000
    end

    return parse_args(s)
end


function main()
    parsed_args = parse_commandline()

    lmax = parsed_args["lmax"]
    n_coeffs = (lmax + 1)^2

    n_samples = parsed_args["samples"]
    @timeit to "samples" begin
        samples_rtp, ylm = Geotools.spherical_samples(n_samples, lmax)

        n_samples = length(samples_rtp)
        points = zeros(Float32, 3, n_samples)
        directions = zeros(Float32, 3, n_samples)
        for i ∈ 1:n_samples
            directions[:, i] = Geotools.rtp2xyz(samples_rtp[i])
        end
    end

    out_rid = open("rid_coeffs.csv", "w")
    write(out_rid, "ligand_id,")
    for l ∈ 0:lmax
        if l < lmax
            line = @sprintf "%d," l
        else
            line = @sprintf "%d\n" l
        end
        write(out_rid, line)
    end

    out_coeffs = open("sh_coeffs.csv", "w")
    write(out_coeffs, "ligand_id,")
    for l ∈ 0:lmax
        for m ∈ -l:l
            if l == lmax && m == l
                line = @sprintf "%d_%d\n" m l
            else
                line = @sprintf "%d_%d," m l
            end
            write(out_coeffs, line)
        end
    end

    in_files = glob("*.tmesh", parsed_args["dir"])
    for in_file_name ∈ in_files
        @show in_file_name

        surface_mesh = Geotools.load_tmesh(in_file_name)

        @timeit to "match" begin
            bounding_spheres = [BSphere{Float32}(tri) for tri in surface_mesh]
            bvh = BVH(bounding_spheres, BBox{Float32}, UInt32)
            traversal = traverse_rays(bvh, points, directions)
        end

        @timeit to "interpol" begin
            samples_radii = zeros(n_samples)
            surface_values = zeros(n_samples)
            for (mesh_idx, ray_idx) ∈ traversal.contacts
                ray = directions[:, ray_idx]

                faces = surface_mesh.faces[mesh_idx]

                triangles = Vector{Vector{Float32}}([surface_mesh.vertex_attributes[:position][face] for face in faces])
                vertex_values = Vector{Float32}([surface_mesh.vertex_attributes[:ESP][face] for face in faces])

                intersects, intersection = Geotools.ray_intersects_triangle(ray, triangles)
                if intersects
                    u, v = Geotools.barycentric_coordinates(triangles, intersection)
                    intersection_rtp = Geotools.xyz2rtp(intersection)
                    samples_radii[ray_idx] = intersection_rtp[1]
                    surface_values[ray_idx] = u * vertex_values[2] +
                                              v * vertex_values[3] +
                                              (1 - u - v) * vertex_values[1]
                end
            end
        end

        # out_file = open("unit_intersect.csv", "w")
        # for i ∈ 1:n_samples
        #     coord = copy(samples_rtp[i])
        #     coord[1] = samples_radii[i]
        #     intersection = Geotools.rtp2xyz(coord)
        #     line = @sprintf "%.5f,%.5f,%.5f,%.5f\n" intersection[1] intersection[2] intersection[3] surface_values[i]
        #     write(out_file, line)
        # end
        # close(out_file)

        @timeit to "SH" begin
            delta = 4.0 * π / n_samples
            rid_surface = Dict{Int64,Float64}()
            rid_surface_values = Dict{Int64,Float64}()
            coeffs_surface = zeros(n_coeffs)
            coeffs_surface_values = zeros(n_coeffs)
            i = 1
            for l ∈ 0:lmax
                sum_surface_coeffs = 0.0
                sum_surface_values_coeffs = 0.0
                for m ∈ -l:l
                    alm_surface = 0.0
                    alm_surface_values = 0.0
                    # Compute the SH coeff alm for a l,m pair
                    for j ∈ 1:n_samples
                        alm_surface += samples_radii[j] * ylm[j][i] * delta
                        alm_surface_values += surface_values[j] * ylm[j][i] * delta
                    end
                    coeffs_surface[i] = alm_surface
                    coeffs_surface_values[i] = alm_surface_values
                    i += 1

                    # RID computation
                    sum_surface_coeffs += alm_surface^2
                    sum_surface_values_coeffs += alm_surface_values^2
                end
                rid_surface[l] = sum_surface_coeffs
                rid_surface_values[l] = sum_surface_values_coeffs
            end
        end

        file_name = splitdir(in_file_name)[2]
        file_id = splitext(file_name)[1]
        write(out_rid, "$(file_id)_shape,")
        for l ∈ 0:lmax
            if l < lmax
                line = @sprintf "%.5f," rid_surface[l]
            else
                line = @sprintf "%.5f\n" rid_surface[l]
            end
            write(out_rid, line)
        end
        write(out_rid, "$(file_id)_esp,")
        for l ∈ 0:lmax
            if l < lmax
                line = @sprintf "%.5f," rid_surface_values[l]
            else
                line = @sprintf "%.5f\n" rid_surface_values[l]
            end
            write(out_rid, line)
        end

        write(out_coeffs, "$(file_id)_shape,")
        i = 1
        for l ∈ 0:lmax
            for m ∈ -l:l
                if l == lmax && m == l
                    line = @sprintf "%.5f\n" coeffs_surface[i]
                else
                    line = @sprintf "%.5f," coeffs_surface[i]
                end
                i += 1
                write(out_coeffs, line)
            end
        end
        write(out_coeffs, "$(file_id)_esp,")
        i = 1
        for l ∈ 0:lmax
            for m ∈ -l:l
                if l == lmax && m == l
                    line = @sprintf "%.5f\n" coeffs_surface_values[i]
                else
                    line = @sprintf "%.5f," coeffs_surface_values[i]
                end
                i += 1
                write(out_coeffs, line)
            end
        end

        # out_file = open("reconstruction.csv", "w")
        # for j ∈ 1:n_samples
        #     r = samples_rtp[j][1]
        #     θ = samples_rtp[j][2]
        #     ϕ = samples_rtp[j][3]

        #     value = surface_values[j]

        #     i = 1
        #     sum_surface = 0.0
        #     sum_surface_values = 0.0
        #     for l ∈ 0:lmax
        #         for m ∈ -l:l
        #             sum_surface += coeffs_surface[i] * ylm[j][i]
        #             sum_surface_values += coeffs_surface_values[i] * ylm[j][i]
        #             i += 1
        #         end
        #     end
        #     line = @sprintf "%.5f,%.5f,%.5f,%.5f,%.5f,%.5f\n" θ ϕ r sum_surface value sum_surface_values
        #     write(out_file, line)
        # end
        # close(out_file)
    end
    close(out_rid)
    close(out_coeffs)
end

main()
show(to)
