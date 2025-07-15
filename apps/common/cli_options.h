// This file is part of otmap, an optimal transport solver.
//
// Copyright (C) 2017 Georges Nader
//
// This Source Code Form is subject to the terms of the Mozilla
// Public License v. 2.0. If a copy of the MPL was not distributed
// with this file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "otsolver_options.h"
#include "inputparser.h"
#include <string>

struct CLIopts : CLI_OTSolverOptions
{
  std::string filename_src;
  bool uniform_src;

  std::string filename_trg;

  std::string output_path;

  bool inv_mode;

  unsigned int resolution;

  double focal_l;
  double thickness;
  double mesh_width;
  double refractive_index;

  void set_default()
  {
    filename_src = "";
    uniform_src = false;
    filename_trg = "";

    output_path = "./output.obj";

    resolution = 100;

    focal_l = 1.0;
    thickness = 0.2;
    mesh_width = 1.0;
    refractive_index = 1.55;

    CLI_OTSolverOptions::set_default();
  }

  bool load(const InputParser &args)
  {
    set_default();

    CLI_OTSolverOptions::load(args);

    std::vector<std::string> value;

    if(args.getCmdOption("-in_src", value))
      filename_src = value[0];
    else
      uniform_src = true;

    if(args.getCmdOption("-in_trg", value))
      filename_trg = value[0];
    else
      return false;

    if(args.getCmdOption("-output", value))
      output_path = value[0];

    if(args.getCmdOption("-res", value))
      resolution = std::atoi(value[0].c_str());

    if(args.getCmdOption("-focal_l", value))
      focal_l = std::atof(value[0].c_str());

    if(args.getCmdOption("-thickness", value))
      thickness = std::atof(value[0].c_str());

    if(args.getCmdOption("-mesh_width", value))
      mesh_width = std::atof(value[0].c_str());

    if(args.getCmdOption("-refractive_index", value) || args.getCmdOption("-ri", value))
      refractive_index = std::atof(value[0].c_str());

    return true;
  }

  // Helper function to get the final output file path
  std::string get_output_file_path() const
  {
    // Check if output_path ends with .obj (is a full file path)
    if (output_path.length() >= 4 && 
        output_path.substr(output_path.length() - 4) == ".obj") {
      return output_path;
    }
    
    // Otherwise treat as directory path
    std::string dir_path = output_path;
    
    // Ensure directory path ends with separator
    if (!dir_path.empty() && dir_path.back() != '/' && dir_path.back() != '\\') {
      dir_path += "/";
    }
    
    return dir_path + "output.obj";
  }
};
