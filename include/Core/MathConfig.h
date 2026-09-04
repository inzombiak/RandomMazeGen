#ifndef CORE_MATH_CONFIG_H
#define CORE_MATH_CONFIG_H

// Single source of truth for the project's GLM configuration.
//
// The renderer is Direct3D 12, so the whole binary uses a LEFT-HANDED basis
// with a [0,1] depth range (see Renderer_D12::UpdateMVP -> glm::lookAtLH /
// glm::perspectiveFovLH).
//
// GLM is header-only. If one translation unit compiles it with these macros
// and another compiles it without, the two emit different definitions of the
// same inline/template functions under identical mangled names -- an ODR
// violation the linker resolves silently and arbitrarily. Symptoms are things
// like a correct-looking fall with a mirrored spin, which read as solver bugs
// rather than basis bugs.
//
// The macros are therefore set project-wide in <PreprocessorDefinitions> for
// every configuration, NOT in individual headers. This file exists so there is
// one place that says why, and so the guard below can catch a TU that slips
// through.
//
// Any code touching GLM should include this header rather than <glm/glm.hpp>
// directly.

#if defined(GLM_SETUP_INCLUDED) && \
    (!defined(GLM_FORCE_LEFT_HANDED) || !defined(GLM_FORCE_DEPTH_ZERO_TO_ONE))
#error "GLM was included before Core/MathConfig.h without the project's handedness/depth macros. \
This is an ODR violation. Ensure GLM_FORCE_LEFT_HANDED and GLM_FORCE_DEPTH_ZERO_TO_ONE are set \
project-wide (see RandomGen.vcxproj / Physics.vcxproj <PreprocessorDefinitions>)."
#endif

#if !defined(GLM_FORCE_LEFT_HANDED) || !defined(GLM_FORCE_DEPTH_ZERO_TO_ONE)
#error "GLM_FORCE_LEFT_HANDED and GLM_FORCE_DEPTH_ZERO_TO_ONE must be defined project-wide."
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#endif // CORE_MATH_CONFIG_H
