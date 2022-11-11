DFT-FE can be interfaced with [MolSSI-MDI](https://github.com/MolSSI-MDI/MDI_Library). The MolSSI Driver Interface (MDI) project provides a standardized API for fast, on-the-fly communication between computational chemistry codes. DFT-FE can be used as a QM engine through MDI and the interface adheres to the [MDI Standard](https://molssi-mdi.github.io/MDI_Library/html/mdi_standard.html). Currently only the TCP communication route has been tested. 


Steps to interface MolSSI-MDI with DFT-FE
==========================================

* Install MDI library from [MolSSI-MDI](https://github.com/MolSSI-MDI/MDI_Library).

* Install DFT-FE current development branch (publicGithubDevelop) from [DFT-FE github repo](https://github.com/dftfeDevelopers/dftfe). The installation instructions for DFT-FE and its dependencies are provied in the development version manual [here](https://github.com/dftfeDevelopers/dftfe/blob/manual/manual-develop.pdf). To link the dftfe library to MDI set the `mdiPath` variable to the MDI installation path in [setupUser.sh](https://github.com/dftfeDevelopers/dftfe/blob/publicGithubDevelop/helpers/setupUser.sh) and also set the variable `withMDI` to `ON`. Please note that two separate libraries will be installed, one for real datatype (Gamma point) and the other for the complex datatype (multiple k-points).


* Create and compile a MDI Driver. This step does not require linking to DFT-FE in the stand-alone TCP and MPI communication routes.  Please refer to MDI library documentation for details regarding setup a driver. We also refer to an [example driver](https://github.com/dsambit/MDI_Library/blob/master/driverTestDFTFE/testcxxstandalone/driver_cxx/driver_cxx.cpp) for ground-state QM calculation that has been tested with DFT-FE as the QM engine. This driver is written in cxx.

* `>COORDS` must be with respect to origin at the cell corner.

* Before using the interface set `DFTFE_PSP_PATH` environment variable using export to a pseudopotential directory. The pseudpotential directory must contain ONCV files in the format: *AtomicSymbol.upf*

* Example usage of MDI in TCP mode using compiled `driver_cxx` and `dftfe` executables
```
mpirun -np 1 driver_cxx -mdi "-role DRIVER -name driver -method TCP -out driver.out -port 8021" &
mpirun -np 36 dftfe -mdi "-role ENGINE -name engine1 -method TCP -out engine1.out -port 8021 -hostname localhost" &
```

* DFT-FE's MDI interface currently only sets up GGA PBE ground-state DFT calculations using ONCV pseudopotentials (http://www.pseudo-dojo.org for example) on either CPU only or hybrid CPU-GPU architecture. Fermi-dirac smearing with 500 K smearing temperature is used by default.

* **CAUTION**: Due to the nature of the electrostatics formulation implemented in DFT-FE we strongly recommend to tile periodic cell lengths so that they are more than 10 atomic units
