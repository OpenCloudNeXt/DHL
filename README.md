# [DHL][dhl]

DHL (Dynamic Hardware Function Libraries) is a high performance FPGA-CPU co-design framework for accelerating software NFs (Network Functions) with [Intel DPDK][dpdk]. 

# About

DPDK is capable of manipulating the network traffic at line-rate of 40 ~ 100 Gbps, for the network functions like L2-forward and L3-forward. 
That indeed improves the power of software NFs, however, when it comes to the complicated NFs containing computation-intensive operations, like IPsec gateway, the only-DPDK/only-CPU solution is facing the price of a lot of CPU cores. 
On the other hand, owing to its high concurrency and programmability, there are a lot of works implement NFs in FPGA. However, the programmable logic blocks on an FPGA board are very limited and expensive. Deploying the entire NF on FPGA is thus resource-demanding. Further, FPGA needs to be reprogrammed when the NF logic changes which can take hours to synthesize the code to generate new programs, hindering the rapid deployment of NFs. 

Under this circumstances, it calls for DHL framework which flexibly offloads server cycles of computation-intensive to FPGA gates. DHL framework abstracts accelerator modules in FPGA as a hardware function library, and provides a set of transparent APIs for software developers. 
Moreover, DHL framework provides orchestration and management of different accelerator modules (hardware functions) among multiple FPGA cards.

If you are interested in DHL framework, detailed description can be got in our [ICDCS 2018][icdcs2018] paper.

# Installation
To install DHL, please see the [DHL Installation][install] guide for a specific walkthrough.

# Using DHL framework
1. Software NF developing

	DHL comes with several [sample network functions][examples]. 
	To get started with these examples, please see the [Example Uses][examples_doc] guide.
	
2. Hardware Accelerator module developing

	There are two pre-designed hardware accelerator modules (ipsec-crypto and pattern-matching) along with DHL framework. 
	Of course, you can customize your own accelerator module based on the requirement of your software NF. 
	Please see the [Customizing Hardware Accelerator][customizing_hw_acc] guide for detailed information.

# Citing DHL
If you use DHL in your work, please cite our paper:
```
@inproceedings{Li2018DHL,
  title={DHL: Enabling Flexible Software Network Functions with FPGA Acceleration},
  author={Li, Xiaoyao and Wang, Xiuxiu and Liu, Fangming and Xu, Hong},
  booktitle={Distributed Computing Systems (ICDCS), 2018 IEEE 38th International Conference on},
  year={2018},
  organization={IEEE}
}
```

_Please inform us if you use DHL in you research by [emailing us](mailto:fmliu@hust.edu.cn)._

[dhl]: http://opencloudnext.github.io/
[dpdk]: http://dpdk.org/
[icdcs2018]: http://grid.hust.edu.cn/fmliu/icdcs18-DHL-FPGA-FangmingLiu.pdf
[install]: docs/Install.md
[examples]: examples/
[examples_doc]: docs/Examples.md
[customizing_hw_acc]: docs/Customizing_hardware_accelerator.md