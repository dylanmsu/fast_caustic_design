/*
 * Copyright (C) 2025 Dylan Missuwe
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <iostream>
#include "common/otsolver_options.h"
#include "common/cli_options.h"
#include "utils/eigen_addons.h"
#include "common/image_utils.h"
#include "common/generic_tasks.h"
#include "utils/BenchTimer.h"
#include <surface_mesh/Surface_mesh.h>
#include "utils/rasterizer.h"


#include "normal_integration/normal_integration.h"
#include "normal_integration/mesh.h"

// GUI support
//#ifdef _WIN32
extern int launchGUI();
//#endif

using namespace Eigen;
using namespace surface_mesh;
using namespace otmap;

void output_usage()
{
  std::cout << "\033[1;36m" << "CAUSTIC DESIGN - 3D Caustic Surface Generator" << "\033[0m" << std::endl;
  std::cout << "Generates 3D lens surfaces that create desired light patterns (caustics)" << std::endl;
  std::cout << std::endl;

  std::cout << "\033[1;33m" << "USAGE:" << "\033[0m" << std::endl;
  std::cout << "  caustic_design -in_trg <target_image> [options]" << std::endl;
  std::cout << std::endl;

  std::cout << "\033[1;33m" << "REQUIRED PARAMETERS:" << "\033[0m" << std::endl;
  std::cout << "  -in_trg <filename>     Target caustic pattern image (what you want the light to look like)" << std::endl;
  std::cout << "                         Supported formats: PNG, BMP, JPG, and other CImg formats" << std::endl;
  std::cout << "                         Must be square (same width and height)" << std::endl;
  std::cout << "                         OR use procedural patterns: :id:resolution: (e.g., :8:256:)" << std::endl;
  std::cout << "                         Available patterns: 1=constant, 5=linear, 6=circles+boundary," << std::endl;
  std::cout << "                         8=circles, 10=single black dot, 11=single white dot," << std::endl;
  std::cout << "                         21-25=research examples, 31-33=advanced patterns, etc." << std::endl;
  std::cout << std::endl;

  std::cout << "\033[1;33m" << "LENS GEOMETRY OPTIONS:" << "\033[0m" << std::endl;
  std::cout << "  -res <value>           Mesh resolution for the caustic surface (default: 100)" << std::endl;
  std::cout << "                         Higher values = more detail but slower computation" << std::endl;
  std::cout << "  -focal_l <value>       Focal length of the caustic lens (default: 1.0)" << std::endl;
  std::cout << "                         Distance from lens to projection plane" << std::endl;
  std::cout << "  -thickness <value>     Physical thickness of the lens (default: 0.2)" << std::endl;
  std::cout << "                         Controls how thick the final 3D model will be" << std::endl;
  std::cout << "  -mesh_width <value>    Physical width and height of the lens (default: 1.0)" << std::endl;
  std::cout << "                         Sets the overall size of the lens" << std::endl;
  std::cout << "  -refractive_index <value> or -ri <value>" << std::endl;
  std::cout << "                         Refractive index of the lens material (default: 1.55)" << std::endl;
  std::cout << "                         Controls how light bends through the lens" << std::endl;
  std::cout << "                         Common materials: Glass=1.50, PLA=1.45, Acrylic=1.49," << std::endl;
  std::cout << "                         Clear Resin=1.51, Higher values = stronger light bending" << std::endl;
  std::cout << std::endl;

  std::cout << "\033[1;33m" << "SOURCE LIGHT OPTIONS:" << "\033[0m" << std::endl;
  std::cout << "  -in_src <filename>     Source light distribution image (optional)" << std::endl;
  std::cout << "                         If not provided, uniform light distribution is used" << std::endl;
  std::cout << "                         Same format requirements as target image" << std::endl;
  std::cout << std::endl;

  std::cout << "\033[1;33m" << "OUTPUT OPTIONS:" << "\033[0m" << std::endl;
  std::cout << "  -output <path>         Output file path (default: './output.obj')" << std::endl;
  std::cout << "                         Can specify full path with filename or just directory" << std::endl;
  std::cout << "                         If directory only, 'output.obj' will be used as filename" << std::endl;
  std::cout << std::endl;

  std::cout << "\033[1;33m" << "SOLVER OPTIONS:" << "\033[0m" << std::endl;
  std::cout << "  -beta <method>         Optimization method: 'cj' (conjugate jacobian) or '0' (zero)" << std::endl;
  std::cout << "                         Default: 'cj' (recommended for better performance)" << std::endl;
  std::cout << "  -itr <max_iterations>  Maximum solver iterations (default: 1000)" << std::endl;
  std::cout << "  -th <threshold>        Convergence threshold (default: 1e-7)" << std::endl;
  std::cout << "                         Lower values = more precise but slower" << std::endl;
  std::cout << "  -ratio <max_ratio>     Maximum target ratio (default: unlimited)" << std::endl;
  std::cout << "  -v <level>             Verbosity level 0-10 (default: 1)" << std::endl;
  std::cout << "                         0=silent, 1=normal, 10=very detailed" << std::endl;
  std::cout << std::endl;

  std::cout << "\033[1;33m" << "HELP:" << "\033[0m" << std::endl;
  std::cout << "  -h, -help              Show this help message" << std::endl;
  std::cout << std::endl;

  std::cout << "\033[1;32m" << "EXAMPLES:" << "\033[0m" << std::endl;
  std::cout << "  Basic usage:" << std::endl;
  std::cout << "    caustic_design -in_trg my_pattern.png" << std::endl;
  std::cout << std::endl;
  std::cout << "  High resolution with custom focal length:" << std::endl;
  std::cout << "    caustic_design -in_trg pattern.png -res 200 -focal_l 2.0" << std::endl;
  std::cout << std::endl;
  std::cout << "  With custom source light and output directory:" << std::endl;
  std::cout << "    caustic_design -in_trg target.png -in_src source.png -output ./results/" << std::endl;
  std::cout << std::endl;
  std::cout << "  Using procedural pattern (circles):" << std::endl;
  std::cout << "    caustic_design -in_trg :8:256: -res 150" << std::endl;
  std::cout << std::endl;
  std::cout << "  Testing with single black dot pattern:" << std::endl;
  std::cout << "    caustic_design -in_trg :10:128: -focal_l 1.5" << std::endl;
  std::cout << std::endl;

  std::cout << "\033[1;34m" << "OUTPUT:" << "\033[0m" << std::endl;
  std::cout << "  The program generates a 3D model file (.obj format) that can be:" << std::endl;
  std::cout << "  * 3D printed to create a physical caustic lens" << std::endl;
  std::cout << "  * Imported into 3D modeling software (Blender, Maya, etc.)" << std::endl;
  std::cout << "  * Used in optical simulations" << std::endl;
  std::cout << std::endl;

  std::cout << "\033[1;35m" << "NOTES:" << "\033[0m" << std::endl;
  std::cout << "  * Images must be square (same width and height)" << std::endl;
  std::cout << "  * Higher resolution (-res) gives more detail but takes longer" << std::endl;
  std::cout << "  * The lens uses refractive index 1.55 (typical plastic/glass)" << std::endl;
  std::cout << "  * Processing involves 10 outer iterations for optimal results" << std::endl;
  std::cout << "  * Procedural patterns are useful for testing - try :8:256: for circles" << std::endl;
  std::cout << "  * Output path can be full filename or directory (defaults to 'output.obj')" << std::endl;
  std::cout << std::endl;
}



template<typename T,typename S>
T lerp(S u, const T& a0, const T& a1)
{
  return (1.-u)*a0 + u*a1;
}

void interpolate(const std::vector<Surface_mesh> &inv_maps, double alpha, Surface_mesh& result)
{
  //clear output
  result.clear();
  result = inv_maps[0];

  int nv = result.vertices_size();

  for(int j=0; j<nv; ++j){
    Surface_mesh::Vertex v(j);
    // linear interpolation
    result.position(v) = lerp(alpha,inv_maps[0].position(v),inv_maps[1].position(v));
  }
}

void synthetize_and_save_image(const Surface_mesh& map, const std::string& filename, int res, double expected_mean, bool inv)
{
  MatrixXd img(res,res);
  rasterize_image(map, img);
  img = img * (expected_mean/img.mean());

  if(inv)
    img = 1.-img.array();

  save_image(filename.c_str(), img);
}

std::vector<double> normalize_vec(std::vector<double> p1) {
    std::vector<double> vec(3);
    double squared_len = 0;
    for (int i=0; i<p1.size(); i++) {
        squared_len += p1[i] * p1[i];
    }

    double len = std::sqrt(squared_len);

    for (int i=0; i<p1.size(); i++) {
        vec[i] = p1[i] / len;
    }

    return vec;
}


// Function to calculate the gradient of f(y, z)
void gradient(  std::vector<double> source,
                std::vector<double> interf,
                std::vector<double> target,
                double n1, double n2,
                double & grad_x, double & grad_y) {
    double d1 = std::sqrt((interf[0] - source[0]) * (interf[0] - source[0]) + (interf[1] - source[1]) * (interf[1] - source[1]) + (interf[2] - source[2]) * (interf[2] - source[2]));
    double d2 = std::sqrt((target[0] - interf[0]) * (target[0] - interf[0]) + (target[1] - interf[1]) * (target[1] - interf[1]) + (target[2] - interf[2]) * (target[2] - interf[2]));

    grad_x = n1 * (interf[0] - source[0]) / d1 - n2 * (target[0] - interf[0]) / d2;
    grad_y = n1 * (interf[1] - source[1]) / d1 - n2 * (target[1] - interf[1]) / d2;
}

void scaleAndTranslatePoints(std::vector<std::vector<double>>& points, double MAX_X, double MAX_Y, double margin) {
    double scaleFactorX = (MAX_X - 2 * margin) / MAX_X;
    double scaleFactorY = (MAX_Y - 2 * margin) / MAX_Y;

    for (auto& point : points) {
        double& x = point[0];
        double& y = point[1];

        x = x * scaleFactorX;
        y = y * scaleFactorY;

        x += margin;
        y += margin;
    }
}

void export_grid_to_svg(std::vector<std::vector<double>> &points, double width, double height, int res_x, int res_y, std::string filename, double stroke_width) {
    std::ofstream svg_file(filename, std::ios::out);
    if (!svg_file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
    }

    // Write SVG header
    svg_file << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    svg_file << "<svg width=\"1000\" height=\"" << 1000.0f * (height / width) << "\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n";

    svg_file << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";

    for (int j = 0; j < res_y; j++) {
        std::string path_str = "M";
        for (int i = 0; i < res_x; i++) {
            int idx = i + j * res_x;

            const auto& point = points[idx];
            path_str += std::to_string((point[0] / width) * 1000.0f) + "," +
                        std::to_string((point[1] / height) * 1000.0f * (height / width));
            if (i < res_x - 1)
                path_str += "L";
        }
        svg_file << "<path d=\"" << path_str << "\" fill=\"none\" stroke=\"black\" stroke-width=\"" << stroke_width << "\"/>\n";
    }

    for (int j = 0; j < res_x; j++) {
        std::string path_str = "M";
        for (int i = 0; i < res_y; i++) {
            int idx = j + i * res_x;

            const auto& point = points[idx];
            path_str += std::to_string((point[0] / width) * 1000.0f) + "," +
                        std::to_string((point[1] / height) * 1000.0f * (height / width));

            if (i < res_x - 1)
                path_str += "L";
        }
        svg_file << "<path d=\"" << path_str << "\" fill=\"none\" stroke=\"black\" stroke-width=\"" << stroke_width << "\"/>\n";
    }

    // Write SVG footer
    svg_file << "</svg>\n";
    svg_file.close();
}

void export_triangles_to_svg(std::vector<std::vector<double>> &points, std::vector<std::vector<unsigned int>> &triangles, double width, double height, int res_x, int res_y, std::string filename, double stroke_width) {
    std::ofstream svg_file(filename, std::ios::out);
    if (!svg_file.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
    }

    // Write SVG header
    svg_file << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    svg_file << "<svg width=\"1000\" height=\"" << 1000.0f * (height / width) << "\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n";

    svg_file << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";

    // Draw polygons
    for (const auto& polygon : triangles) {
        std::vector<std::vector<double>> poly_points;
        for (const auto& point_idx : polygon) {
            poly_points.push_back(points[point_idx]);
        }

        std::string path_str = "M";
        for (std::size_t j = 0; j < poly_points.size(); ++j) {
            const auto& point = poly_points[j];
            path_str += std::to_string((point[0] / width) * 1000.0f) + "," +
                        std::to_string((point[1] / height) * 1000.0f * (height / width));

            if (j < poly_points.size() - 1)
                path_str += "L";
        }
        path_str += "Z";
        svg_file << "<path d=\"" << path_str << "\" fill=\"none\" stroke=\"black\" stroke-width=\"" << stroke_width << "\"/>\n";
    }

    // Write SVG footer
    svg_file << "</svg>\n";
    svg_file.close();
}


void scalePoints(std::vector<std::vector<double>>& trg_pts, const std::vector<double>& scale, const std::vector<double>& origin) {
    for (std::vector<double>& point : trg_pts) {
        for (size_t j = 0; j < point.size(); ++j) {
            // Scale each dimension relative to the origin
            point[j] = origin[j] + (point[j] - origin[j]) * scale[j];
        }
    }
}

void translatePoints(std::vector<std::vector<double>>& trg_pts, std::vector<double> position_xyz) {
  for (int i = 0; i < trg_pts.size(); i++)
  {
    trg_pts[i][0] += position_xyz[0];
    trg_pts[i][1] += position_xyz[1];
    trg_pts[i][2] += position_xyz[2];
  }
}

// Define the rotation function
void rotatePoints(std::vector<std::vector<double>>& trg_pts, std::vector<double> angle_xyz) {
    double PI = 3.14159265358979323846;

    // Convert angles from degrees to radians
    angle_xyz[0] = angle_xyz[0] * PI / 180.0;
    angle_xyz[1] = angle_xyz[1] * PI / 180.0;
    angle_xyz[2] = angle_xyz[2] * PI / 180.0;

    // Precompute sine and cosine values for each rotation angle
    double cos_x = std::cos(angle_xyz[0]);
    double sin_x = std::sin(angle_xyz[0]);
    double cos_y = std::cos(angle_xyz[1]);
    double sin_y = std::sin(angle_xyz[1]);
    double cos_z = std::cos(angle_xyz[2]);
    double sin_z = std::sin(angle_xyz[2]);

    // Define the rotation matrices for each axis
    std::vector<std::vector<double>> rot_x = {
        {1, 0, 0},
        {0, cos_x, -sin_x},
        {0, sin_x, cos_x}
    };

    std::vector<std::vector<double>> rot_y = {
        {cos_y, 0, sin_y},
        {0, 1, 0},
        {-sin_y, 0, cos_y}
    };

    std::vector<std::vector<double>> rot_z = {
        {cos_z, -sin_z, 0},
        {sin_z, cos_z, 0},
        {0, 0, 1}
    };

    // Apply rotation to each point in the point cloud
    for (std::vector<double>& point : trg_pts) {
        // Extract x, y, z for clarity
        double x = point[0];
        double y = point[1];
        double z = point[2];

        // Rotate around x-axis
        double new_y = rot_x[1][1] * y + rot_x[1][2] * z;
        double new_z = rot_x[2][1] * y + rot_x[2][2] * z;
        y = new_y;
        z = new_z;

        // Rotate around y-axis
        double new_x = rot_y[0][0] * x + rot_y[0][2] * z;
        new_z = rot_y[2][0] * x + rot_y[2][2] * z;
        x = new_x;
        z = new_z;

        // Rotate around z-axis
        new_x = rot_z[0][0] * x + rot_z[0][1] * y;
        new_y = rot_z[1][0] * x + rot_z[1][1] * y;
        x = new_x;
        y = new_y;

        // Update the point with the rotated coordinates
        point[0] = x;
        point[1] = y;
        point[2] = z;
    }
}

TransportMap runOptimalTransport(MatrixXd &density, CLIopts &opts) {
  GridBasedTransportSolver otsolver;
  otsolver.set_verbose_level(opts.verbose_level-1);

  if(opts.verbose_level>=1)
    std::cout << "Generate transport map...\n";

  if(density.maxCoeff()>1.)
    density = density / density.maxCoeff(); //normalize

  BenchTimer t_solver_init, t_solver_compute, t_generate_uniform;

  t_solver_init.start();
  otsolver.init(static_cast<int>(density.rows()));
  t_solver_init.stop();

  std::cout << "init\n";

  t_solver_compute.start();
  TransportMap tmap_src = otsolver.solve(vec(density), opts.solver_opt);
  t_solver_compute.stop();

  std::cout << "STATS solver -- init: " << t_solver_init.value(REAL_TIMER) << "s  solve: " << t_solver_compute.value(REAL_TIMER) << "s\n";

  return tmap_src;
}

void applyTransportMapping(TransportMap &tmap_src, TransportMap &tmap_trg, MatrixXd &density_trg, std::vector<Eigen::Vector2d> &vertex_positions) {
  Surface_mesh map_uv = tmap_src.fwd_mesh();
  Surface_mesh map_orig = tmap_src.origin_mesh();

  apply_inverse_map(tmap_trg, map_uv.points(), 3);

  auto originMeshPtr = std::make_shared<surface_mesh::Surface_mesh>(map_uv);
  auto fwdMeshPtr = std::make_shared<surface_mesh::Surface_mesh>(map_orig);
  auto densityPtr = std::make_shared<Eigen::VectorXd>(density_trg);

  TransportMap transport(originMeshPtr, fwdMeshPtr, densityPtr);

  apply_inverse_map(transport, vertex_positions, 3);
}

std::vector<double> cross(std::vector<double> v1, std::vector<double> v2){
  std::vector<double> result(3);
  result[0] = v1[1]*v2[2] - v1[2]*v2[1];
  result[1] = v1[2]*v2[0] - v1[0]*v2[2];
  result[2] = v1[0]*v2[1] - v1[1]*v2[0];
  return result;
}

double dot(std::vector<double> a, std::vector<double> b) {
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

std::vector<double> mult(double a, std::vector<double> b) {
  return {a*b[0], a*b[1], a*b[2]};
}

std::vector<double> add(std::vector<double> a, std::vector<double> b) {
  return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

std::vector<double> sub(std::vector<double> a, std::vector<double> b) {
  return {a[0] - b[0], a[1] - b[1], a[2] - b[2]};
}

double magnitude(std::vector<double> a) {
  return std::sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
}

// https://stackoverflow.com/questions/29758545/how-to-find-refraction-vector-from-incoming-vector-and-surface-normal
/*std::vector<double> refract(const std::vector<double>& normal, const std::vector<double>& incident, double n1, double n2) {
    // Ratio of refractive indices
    const double n = n1 / n2;
    // Calculate cos(theta_i), assuming normal and incident are unit vectors
    const double cosI = -dot(normal, incident);
    // Calculate sin^2(theta_t) using Snell's law
    const double sinT2 = n * n * (1.0 - cosI * cosI);

    // Check for Total Internal Reflection (TIR)
    if (sinT2 > 1.0) {
        // TIR occurs; return an invalid vector or handle appropriately
        // return Vector::invalid; // Uncomment if you have a way to represent TIR
    }

    // Calculate cos(theta_t)
    const double cosT = sqrt(1.0 - sinT2);
    // Calculate the refracted direction vector
    return add(mult(n, incident), mult((n * cosI - cosT), normal));
}*/

std::vector<double> refract(
    const std::vector<double>& surfaceNormal,
    const std::vector<double>& rayDirection,
    double n1,  // Index of refraction of the initial medium
    double n2   // Index of refraction of the second medium
) {
    // Check that both vectors have three components
    if (surfaceNormal.size() != 3 || rayDirection.size() != 3) {
        throw std::invalid_argument("Vectors must have exactly three components.");
    }

    // Calculate the ratio of indices of refraction
    double nRatio = n1 / n2;

    // Calculate the dot product of surfaceNormal and rayDirection
    double dotProduct = surfaceNormal[0] * rayDirection[0] +
                        surfaceNormal[1] * rayDirection[1] +
                        surfaceNormal[2] * rayDirection[2];

    // Determine the cosine of the incident angle
    double cosThetaI = -dotProduct;  // Cosine of the angle between the ray and the normal

    // Calculate sin^2(thetaT) using Snell's Law
    double sin2ThetaT = nRatio * nRatio * (1.0 - cosThetaI * cosThetaI);

    // Compute cos(thetaT) for the refracted angle
    double cosThetaT = std::sqrt(1.0 - sin2ThetaT);

    // Calculate the refracted ray direction
    std::vector<double> refractedRay(3);
    for (int i = 0; i < 3; ++i) {
        refractedRay[i] = nRatio * rayDirection[i] +
                          (nRatio * cosThetaI - cosThetaT) * surfaceNormal[i];
    }

    return refractedRay;
}

// https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-plane-and-ray-disk-intersection.html
bool intersect_plane(const std::vector<double> &n, const std::vector<double> &p0, const std::vector<double> &l0, const std::vector<double> &l, std::vector<double> &intersectionPoint) {
    double denom = dot(n, l);
    if (denom > 1e-6) { // Check if ray is not parallel to the plane
        std::vector<double> p0l0 = sub(p0, l0);
        double t = dot(p0l0, n) / denom;
        if (t >= 0) { // Check if intersection is in the positive direction of the ray
            intersectionPoint[0] = l0[0] + t * l[0];
            intersectionPoint[1] = l0[1] + t * l[1];
            intersectionPoint[2] = l0[2] + t * l[2];
            return true;
        }
    }
    return false;
}

std::vector<double> calc_plane_normal(const std::vector<double> &A, const std::vector<double> &B, const std::vector<double> &C) {
    std::vector<double> edge1 = sub(B, A);
    std::vector<double> edge2 = sub(C, A);
    std::vector<double> normal = cross(edge1, edge2);
    return normalize_vec(normal); // Normalize the result to get a unit normal
}

bool is_boundary_vertex(Mesh &mesh, std::vector<std::pair<int, int>> &adjacent_edges, std::vector<int> &adjacent_triangles, int vertex_index, std::vector<std::pair<int, int>>& boundary_edges) {
    std::unordered_map<std::pair<int, int>, int, HashPair> edge_triangle_count;
    for (int triangle_index : adjacent_triangles) {
        const std::vector<unsigned int>& triangle = mesh.triangles[triangle_index];
        for (int j = 0; j < 3; ++j) {
            int v1 = triangle[j];
            int v2 = triangle[(j + 1) % 3];
            std::pair<int, int> edge = std::make_pair(std::min(v1, v2), std::max(v1, v2));
            edge_triangle_count[edge]++;
        }
    }

    bool is_boundary = false;
    for (const auto& edge : adjacent_edges) {
        if (edge_triangle_count[edge] == 1) { // Boundary edge
            boundary_edges.push_back(edge);
            is_boundary = true;
        }
    }

    return is_boundary;
}

void project_onto_boundary(std::vector<double> &point) {
  point[0] -= 0.5;
  point[1] -= 0.5;

  double dist = sqrt(pow(point[0], 2) + pow(point[1], 2))*2;

  point[0] /= dist;
  point[1] /= dist;

  point[0] += 0.5;
  point[1] += 0.5;
}

//compute the desired normals
std::vector<std::vector<double>> fresnelMapping(
  std::vector<std::vector<double>> &vertices,
  std::vector<std::vector<double>> &target_pts,
  double refractive_index
) {
    std::vector<std::vector<double>> desiredNormals;

    //double boundary_z = -0.1;

    //vector<std::vector<double>> boundary_points;

    bool use_point_src = false;
    bool use_reflective_caustics = false;

    std::vector<double> pointLightPosition(3);
    pointLightPosition[0] = 0.5;
    pointLightPosition[1] = 0.5;
    pointLightPosition[2] = 0.5;

    // place initial points on the refractive surface where the light rays enter the material
    /*if (use_point_src && !use_reflective_caustics) {
        for(int i = 0; i < vertices.size(); i++) {
            std::vector<double> boundary_point(3);

            // ray to plane intersection to get the initial points
            double t = ((boundary_z - pointLightPosition[2]) / (vertices[i][2] - pointLightPosition[2]));
            boundary_point[0] = pointLightPosition[0] + t*(vertices[i][0] - pointLightPosition[0]);
            boundary_point[1] = pointLightPosition[1] + t*(vertices[i][1] - pointLightPosition[1]);
            boundary_point[2] = boundary_z;
            boundary_points.push_back(boundary_point);
        }
    }*/

    // run gradient descent on the boundary points to find their optimal positions such that they satisfy Fermat's principle
    /*if (!use_reflective_caustics && use_point_src) {
        for (int i=0; i<boundary_points.size(); i++) {
            for (int iteration=0; iteration<100000; iteration++) {
                double grad_x;
                double grad_y;
                gradient(pointLightPosition, boundary_points[i], vertices[i], 1.0, refractive_index, grad_x, grad_y);

                boundary_points[i][0] -= 0.1 * grad_x;
                boundary_points[i][1] -= 0.1 * grad_y;

                // if magintude of both is low enough
                if (grad_x*grad_x + grad_y*grad_y < 0.000001) {
                    break;
                }
            }
        }
    }*/

    for(int i = 0; i < vertices.size(); i++) {
        std::vector<double> incidentLight(3);
        std::vector<double> transmitted = {
            target_pts[i][0] - vertices[i][0],
            target_pts[i][1] - vertices[i][1],
            target_pts[i][2] - vertices[i][2]
        };

        if (use_point_src) {
            incidentLight[0] = vertices[i][0] - pointLightPosition[0];
            incidentLight[1] = vertices[i][1] - pointLightPosition[1];
            incidentLight[2] = vertices[i][2] - pointLightPosition[2];
        } else {
            incidentLight[0] = 0;
            incidentLight[1] = 0;
            incidentLight[2] = -1;
        }

        transmitted = normalize_vec(transmitted);
        incidentLight = normalize_vec(incidentLight);

        std::vector<double> normal(3);
        if (use_reflective_caustics) {
            normal[0] = ((transmitted[0]) + incidentLight[0]) * 1.0f;
            normal[1] = ((transmitted[1]) + incidentLight[1]) * 1.0f;
            normal[2] = ((transmitted[2]) + incidentLight[2]) * 1.0f;
        } else {
            normal[0] = ((transmitted[0]) - (incidentLight[0]) * refractive_index) * -1.0f;
            normal[1] = ((transmitted[1]) - (incidentLight[1]) * refractive_index) * -1.0f;
            normal[2] = ((transmitted[2]) - (incidentLight[2]) * refractive_index) * -1.0f;
        }

        normal = normalize_vec(normal);

        desiredNormals.push_back(normal);
    }

    return desiredNormals;
}

Eigen::MatrixXd rotate90ClockwiseAndFlipX(const Eigen::MatrixXd& mat) {
    int rows = static_cast<int>(mat.rows());
    int cols = static_cast<int>(mat.cols());

    Eigen::MatrixXd rotated(cols, rows);  // New matrix with swapped dimensions

    // 90-degree clockwise rotation + X flip using a loop
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            rotated(j, rows - 1 - i) = mat(i, j);
        }
    }

    return rotated;
}

Eigen::MatrixXd scaleAndTranslate(const Eigen::MatrixXd& mat, double newMin, double newMax) {
    double oldMin = mat.minCoeff();  // Find the minimum value in the matrix
    double oldMax = mat.maxCoeff();  // Find the maximum value in the matrix

    if (oldMin == oldMax) {  // Avoid division by zero if all elements are the same
        return Eigen::MatrixXd::Constant(mat.rows(), mat.cols(), newMin);
    }

    // Apply the scaling formula: newValue = newMin + (oldValue - oldMin) * (newMax - newMin) / (oldMax - oldMin)
    Eigen::MatrixXd scaled = (mat.array() - oldMin) * (newMax - newMin) / (oldMax - oldMin) + newMin;

    return scaled;
}

// Extracted processing function for use by both CLI and GUI
int processCausticGeneration(CLIopts opts) {
  MatrixXd density_src;
  MatrixXd density_trg;

  std::vector<Eigen::Vector2d> vertex_positions;
  normal_integration normal_int;

  if (!opts.uniform_src) {
    // load source image. TODO: assume uniform density if no image is loaded
    if(!load_input_density(opts.filename_src, density_src))
    {
      std::cout << "Failed to load input \"" << opts.filename_src << "\" -> abort.";
      exit(EXIT_FAILURE);
    }
  }

  // load the target image
  if(!load_input_density(opts.filename_trg, density_trg))
  {
    std::cout << "Failed to load input \"" << opts.filename_trg << "\" -> abort.";
    exit(EXIT_FAILURE);
  }

  // create triangle mesh that we want to deform into the caustic surface later
  //Mesh mesh(1.0, 1.0/2, opts.resolution, (int)(opts.resolution/2));
  Mesh mesh(1.0, 1.0, opts.resolution, opts.resolution);

  // precompute triangle connectivity information
  mesh.build_vertex_to_triangles();

  // initialize normal integrator
  normal_int.initialize_data(mesh);
  
  //export_triangles_to_svg(mesh.source_points, mesh.triangles, 1, 1, opts.resolution, opts.resolution, "../triangles.svg", 0.5);
  //export_grid_to_svg(mesh.source_points, 1, 1, opts.resolution, opts.resolution, "../grid.svg", 0.5);

  // scale the mesh such that there is a small margin around the boundary
  scaleAndTranslatePoints(mesh.source_points, opts.mesh_width, opts.mesh_width, opts.mesh_width / opts.resolution);

  // extract the x and y coordinates from the surface vertices
  for (int i=0; i<mesh.source_points.size(); i++)
  {
    Eigen::Vector2d point = {mesh.source_points[i][0], mesh.source_points[i][1]};
    vertex_positions.push_back(point);
  }

  if (!opts.uniform_src) {
    // Rotate matrices with an additional X flip
    Eigen::MatrixXd rotated_src = rotate90ClockwiseAndFlipX(density_src);
    Eigen::MatrixXd rotated_trg = rotate90ClockwiseAndFlipX(density_trg);

    // normalize the images so the brightness is between 0 and 1
    rotated_src = scaleAndTranslate(rotated_src, 0.0, 1.0);
    rotated_trg = scaleAndTranslate(rotated_trg, 0.0, 1.0);

    // Pass the properly rotated images to the optimal transport solver
    TransportMap tmap_src = runOptimalTransport(rotated_src, opts); // computes T(v->1)
    TransportMap tmap_trg = runOptimalTransport(rotated_trg, opts); // computes T(u->1)    
    
    // this moves the points along T(v->u)
    applyTransportMapping(tmap_src, tmap_trg, density_trg, vertex_positions);
  } else {
    // Rotate matrices with an additional X flip
    Eigen::MatrixXd rotated_trg = rotate90ClockwiseAndFlipX(density_trg);

    // normalize the images so the brightness is between 0 and 1
    rotated_trg = scaleAndTranslate(rotated_trg, 0.0, 1.0);
    
    // Pass the properly rotated images to the optimal transport solver
    TransportMap tmap_trg = runOptimalTransport(rotated_trg, opts); // computes T(u->1) 

    // this moves the points along T(1->u)
    apply_inverse_map(tmap_trg, vertex_positions, 3);
  }

  // turn the moved 2d cordinates back into 3d points
  std::vector<std::vector<double>> trg_pts;
  for (int i=0; i<mesh.source_points.size(); i++)
  {
    std::vector<double> point = {vertex_positions[i].x(), vertex_positions[i].y(), 0};
    trg_pts.push_back(point);
  }

  // trg_pts now contains a point cloud where each 3d point is the target point of each light ray exiting the vertices of the mesh

  //export_grid_to_svg(trg_pts, 1, 0.5, opts.resolution, opts.resolution, "../grid.svg", 0.5);

  std::vector<std::vector<double>> desired_normals;

  // here we apply some transformations to the target points

  // scaling will make the resulting image larger or smaller than the lens itself
  //scalePoints(trg_pts, {8, 8, 0}, {0.5, 0.5, 0});

  // rotation is for example if you want to project an image on an angled surface relative to the lens
  rotatePoints(trg_pts, {0, 0, 0});

  // here we translate the points to the focal plane. you can change the x and y offsets aswel if you application disires it
  translatePoints(trg_pts, {0, 0, -opts.focal_l});

  // refractive index
  double r = opts.refractive_index;

  // precumpute laplacians for the regularization if the normal integration
  mesh.calculate_vertex_laplacians();

  // we perform multiple outer iterations because when the caustic surface changes shape, so do the disired normals of the surface
  for (int i=0; i<10; i++)
  {
      // compute the max z of the caustic surface
      double max_z = -10000;
      for (int j = 0; j < mesh.source_points.size(); j++) {
        if (max_z < mesh.source_points[j][2]) {
          max_z = mesh.source_points[j][2];
        }
      }

      // snap the points so that the max z is 0 each outer iteration
      for (int j = 0; j < mesh.source_points.size(); j++) {
          mesh.source_points[j][2] -= max_z;
      }

      // given the final target points, we compute the vertex normal that steers light towards the target points
      std::vector<std::vector<double>> normals = fresnelMapping(mesh.source_points, trg_pts, r);

      // solve the mesh surface to align with the calculated target normals
      normal_int.perform_normal_integration(mesh, normals);
  }

  // save obj
  std::string final_output_path = opts.get_output_file_path();
  mesh.save_solid_obj_source(opts.thickness, final_output_path);
  std::cout << "\033[1;32m" << "Exported 3d model as " << final_output_path << " relative to this executable." << "\033[0m" << std::endl;

  return 0;
}

int main(int argc, char** argv)
{
  setlocale(LC_ALL,"C");

  // If no arguments provided, launch GUI (Windows only for now)
#ifdef _WIN32
  if (argc == 1) {
    return launchGUI();
  }
#endif

  // parse command line options
  InputParser input(argc, argv);

  if(input.cmdOptionExists("-help") || input.cmdOptionExists("-h")){
    output_usage();
    return 0;
  }

  CLIopts opts;
  if(!opts.load(input)){
    std::cerr << "invalid input" << std::endl;
    output_usage();
    return EXIT_FAILURE;
  }

  // Call the extracted processing function
  return processCausticGeneration(opts);
}
