## Distributed Task Processing (CPP)

Distributed task aims at solving task execution in a distributed fashion where one task can 
process the output of a previous task using any programming language deployed with no 
locality restictions for hosts



### Goals 
- Provide a simple interface to consume and process input data
- Provide a simple producer interface to publish data to be processed
- Language agnostic processing
- Multiple programing languages can process the input data in parallel in publish the results
- Near real time consumption of data


### Supported Adapters:
- Redis (streams)


### Support Languages
- [x] C++ 17 [redis]
- [ ] Python
- [ ] Rust
- [ ] Javascript

### TODO
- [x] Establish an interface to redis using C++ 17
- [x] Basic Stream datastructures creation
- [x] Stream, groups and consumer information gathering
- [x] Consumption interface for processing pending and new messages
- [x] Result publishing to output
- [x] Error publishing to error output
- [ ] Stream length control mechnism
- [ ] Write examples for Readme
- [ ] Illistrations/Diagrams for Readme
- [ ] Concurrency support for stream processing
- [ ] Reclaim pending messages from consumer if they exceed some idle time
- [ ] Collect processing errors by consumer given the data
- [ ] Reprocess error messages within given retries
- [ ] Explore better cpp templates
- [ ] Library packaging and publishing
- [ ] Setup CI/DI pipeline for builds
- [ ] Setup semantic versioning
- [ ] Web based visualization tools for tasks


### Installing
TODO


#### Dependencies
- redis++ 
- fmt
- spdlog


#### Package Managers
- Conan
### Author:
- Muhamad Harris (harris.perceptron@gmail.com)
