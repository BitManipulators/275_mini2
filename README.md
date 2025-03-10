# Build and Execution Instructions

## 1. Extract collisions csv file to top-level of git repo
```
tar -xvf <data.tar.gz>
```

## 2. Build code
```
cd phase_<X>
mkdir build
cd build

# Optionally set the compiler with env vars or lmod
export CC=<your c compiler>
export CXX=<your c++ compiler>

# Use cmake to build project
cmake ..
make
```

## 3. Use ctest to execute unit tests
```
ctest
```

## 4. Execute benchmarks
```
./collision_manager_benchmarks
```

## 5. Run Server
```
./server
```

## 6. Run C++ Client
```
./client
```

## 7. Run Python Client
```
./client.py
```

## 8. Run Yaml parser to get config details
```
./yaml_parser
```