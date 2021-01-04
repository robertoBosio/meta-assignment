# Assignment
Yocto layer for Raspberry Pi 3 implementing a Linux character-based driver and an application which exchanges data with the cDD.
It works also with qemuarm.
## Installation
Inside the poky root directory, type:
```
git clone https://github.com/Binomiale/meta-assignment
```
After that, type:
```
source oe-init-build-env build_rpi3
```
Now it is possible to add the new layer to the image configuration:
```
bitbake-layers add-layer ../meta-assignment/
```
Finally, to add the recipes to the image to be deployed, edit conf/local.conf by adding the following lines:
```
IMAGE_INSTALL_append = " ppgmod"
IMAGE_INSTALL_append = " app"
KERNEL_MODULE_AUTOLOAD += "ppgmod"
```
Now it is possible to build the image:
```
bitbake core-image-full-cmdline
```
## Run the Linux application
On Raspberry Pi 3, after the login, type the command:
```
app
```
It will display every 41 seconds the current BPM.
To properly stop the application use ctrl-C.
