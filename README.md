## Adaptive Duplicate Suppression for Multicasting in a Multi-Access NDN Network

This project implements an adaptive duplicate suppresion for multicasting in NDN.


### Prerequisite

- nfd
- ndn-cxx
- Mini-NDN (optional, only for experiments)

### Installation

#### If using Mini-NDN
- Install Mini-NDN, do the --source installation
- Installation instruction: [Mini-NDN installation guide](https://github.com/named-data/mini-ndn/blob/master/docs/install.rst#using-installsh)

Once Mini-NDN is installed
 - `cd <your minindn installation directory>/dl/nfd`
 - Copy the flies in `ndn-src/` of this repository to `nfd/daemon/face`
 - ./waf configure & ./waf build & sudo ./waf install


#### If not using Mini-NDN

- Installing ndn-cxx 
  - ndn-cxx installation guide: [here](https://github.com/named-data/ndn-cxx/blob/master/docs/INSTALL.rst)
  
- Installing nfd
  - nfd installation guide: [here](https://github.com/named-data/nfd/blob/master/docs/INSTALL.rst)
  - Copy the flies in `ndn-src/` of this repository to `nfd/daemon/face`
  - go to nfd folder (e.g. `cd nfd`)
  - ./waf configure & ./waf build & sudo ./waf install
  
  
More detail about this work: 

Dulal, Saurab, and Lan Wang. "Adaptive duplicate suppression for multicasting in a multi-access NDN network." Proceedings of the 9th ACM Conference on Information-Centric Networking. 2022.

PDF: [here](http://web0.cs.memphis.edu/~lanwang/paper/suppression.pdf) 
  
  






