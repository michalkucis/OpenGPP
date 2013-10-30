OpenGPP is graphical open-source library that allows simple applying post-process effects in OpenGL applications. Besides the library, the project contains self-standing application that allows applying post-process effects to images and video. The applying of effects are performed in GPU using OpenGL and OpenCL library. Primary the project is focused to the simulation of camera features (lens and sensors) but it can be easily extended to simulate another effects. The current version of the post-processor supports following effects:

depth of field effect
using simulated diffusion
reverse-mapped Z-buffer using regular sampling
reverse-mapped Z-buffer using importance sampling
motion blur
geometric distortion
chromatic aberration
camera vignetting
simple inner lens reflections
exposure adaptation
noise
Detailed description of the post-processing and performing of the effects can be found in the paper. Besides post-processing library, the project contains an application that allows performing post-processing effects via modification of images or video.
