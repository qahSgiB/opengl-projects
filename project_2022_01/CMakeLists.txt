################################################################################
# Common Framework for Computer Graphics Courses at FI MUNI.
#
# Copyright (c) 2021-2022 Visitlab (https://visitlab.fi.muni.cz)
# All rights reserved.
#
# Course: PV227 (Project 2022 #1)
################################################################################

# Generates the lecture.
visitlab_generate_lecture(PV227 project_2022_01 EXTRA_FILES src/firework.hpp src/firework.cpp src/math_util.hpp src/math_util.cpp src/ubo_vector.hpp)
# [todo] shoud src/ubo_vector.hpp be here? (header only)