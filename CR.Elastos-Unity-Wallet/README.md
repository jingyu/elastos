# react-native-elastos-mainchain

## Getting started

`$ npm install react-native-elastos-mainchain --save`

### Mostly automatic installation

`$ react-native link react-native-elastos-mainchain`

### Manual installation


#### iOS

1. In XCode, in the project navigator, right click `Libraries` ➜ `Add Files to [your project's name]`
2. Go to `node_modules` ➜ `react-native-elastos-mainchain` and add `RNElastosMainchain.xcodeproj`
3. In XCode, in the project navigator, select your project. Add `libRNElastosMainchain.a` to your project's `Build Phases` ➜ `Link Binary With Libraries`
4. Run your project (`Cmd+R`)

#### Android

TODO

1. Open up `android/app/src/main/java/[...]/MainApplication.java`
  - Add `import org.elastos.mainchain.reactnative.RNElastosMainchainPackage;` to the imports at the top of the file
  - Add `new RNElastosMainchainPackage()` to the list returned by the `getPackages()` method
2. Append the following lines to `android/settings.gradle`:
  	```
  	include ':react-native-elastos-mainchain'
  	project(':react-native-elastos-mainchain').projectDir = new File(rootProject.projectDir, 	'../node_modules/react-native-elastos-mainchain/android')
  	```
3. Insert the following lines inside the dependencies block in `android/app/build.gradle`:
  	```
      compile project(':react-native-elastos-mainchain')
  	```

## Additional configurations

As the react-native-elastos-mainchain package refers to the https://github.com/elastos/Elastos.ORG.Wallet.Lib.C library, some additional configurations are required:
1. Open the Xcode project located under the ios/ folder in your react-native project
2. Click the name of the project and select the target with the same name
3. Click "Build settings" and add "$(SRCROOT)/../node_modules/react-native-elastos-mainchain/Elastos.ORG.Wallet.Lib.C/src" to Header Search Paths
4. Similarly add "$(SRCROOT)/../node_modules/react-native-elastos-mainchain/Elastos.ORG.Wallet.Lib.C/build/ios/src" to Library Search Paths
5. Go to Build Phases/ Link Binary with Libraries and drag&drop the node_modules/react-native-elastos-mainchain/Elastos.ORG.Wallet.Lib.C/build/ios/src/libElastos.Wallet.Utility.dylib file


## Usage
```javascript
import RNElastosMainchain from 'react-native-elastos-mainchain';

// Generate a new mnemonic (defaults to English)
RNElastosMainchain.generateMnemonic((err, mnemonic) => {
    console.log(mnemonic)
});
```
