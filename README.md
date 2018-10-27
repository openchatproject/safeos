The Safe EOS: Secure Multi-Language Smart Contract Platform Base on EOSIO 

# Building safeos

## Downloading Source Code

```bash
git clone https://www.github.com/openchatproject/safeos
cd safeos
git submodule update --init --recursive
```

## Installing dependencies (Ubuntu)

```
sudo apt-get install libleveldb-dev
sudo apt-get install libreadline-dev
```


## Installing dependencies (Centos)

```
sudo yum install leveldb-devel
sudo yum install readline-devel
sudo yum install libffi-devel
```

## Installing dependencies (macOS)

```
brew install leveldb
brew install readline
```

## Building

```bash
./eosio_build.sh
```

