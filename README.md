World smart contract

# Build

```cd <smart_contract_directory>
git clone
go to directory
md build
cd build
cmake ..
make
```

# Update contract

While testing in testnet:
cleos -u https://testnet.waxsweden.org set contract clashdomewld ./clashdomewld -p clashdomewld@active
cleos -u https://tapiwax.3dkrender.com set contract clashdomewld ./clashdomewld -p clashdomewld@active
cleos -u https://testnet.wax.eosdetroit.io set contract clashdomewld ./clashdomewld -p clashdomewld@active

In production:
cleos -u https://api.waxsweden.org set contract clashdomewld ./clashdomewld -p clashdomewld@active