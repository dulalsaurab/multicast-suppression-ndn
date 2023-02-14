### Adaptive duplicate suppression for multicasting in NDN

### Installation
* Clone this repository `git clone https://github.com/dulalsaurab/multicast-suppression-ndn.git`
    * `git branch -a` to see all branches
* Switch to `rl` branch `git checkout rl` 
* **Install NFD**
   * NFD is from the repository `https://github.com/dulalsaurab/NFD`
   * This version of NFD contains files needed for multicast suppression
   * [NFD installation guide](https://github.com/dulalsaurab/NFD/blob/master/docs/INSTALL.rst)

* We use tensorflow for reinforcement learning, thus need to install tensorflow and it's dependencies
* [Tensorflow installation guide](https://www.tensorflow.org/install)

#### Testing Installation
* RL module and NFD suppression module is run independently, thus we can test both of them independently
* `main.py` should run without error
* NFD should compile without error

#### Install Mini-NDN for testing/experiments
* [Mini-NDN installation guide](https://github.com/named-data/mini-ndn/blob/master/docs/install.rst)

### Testing




### Related Paper
Dulal, Saurab, and Lan Wang. "Adaptive duplicate suppression for multicasting in a multi-access NDN network." Proceedings of the 9th ACM Conference on Information-Centric Networking. 2022.
