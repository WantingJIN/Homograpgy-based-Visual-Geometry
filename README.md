# Homograpgy-based-Visual-Geometry2

The goal of this lab is to perform Augmented Reality (AR) from a sequence of images.
The only information that we have is that the camera is observing a mostly planar environment, where we want to display the 3D model of a robot (AR).

Thus, we need to estimate the camera current position in the world frame. We can do this by using the homograpghy relationship of the current frame with respect to the template frame. You can see the detailed theory and process in the \subject floder.

This project is the class project for the Advanced Visual Geometry course in Ecole Centrale de Nantes(ECN). The framework of the project is provided by the professor Olivier Kermorgant from ECN. The student are asked to modified the main code to do the image processing, estimation of the homography matrix and calculate the coordinates of current camera position.

The framework code can be download from Olivier Kermorgant gitub website: https://github.com/oKermorgant/ecn_visualodom
