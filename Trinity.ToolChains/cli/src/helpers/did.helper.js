const os = require("os")
const path = require("path")
const fs = require("fs-extra")
const jwt = require("jsonwebtoken")
const QRCode = require('qrcode')
const open = require('open')
const Spinner = require('cli-spinner').Spinner
const axios = require("axios")
const sleep = require("await-sleep")
const prompts = require("prompts")

module.exports = class DIDHelper {
    /**
     * Creates a new DID in current folder.
     */
    createDID() {
        return new Promise(async (resolve, reject) => {
            console.log("Creating a new DID...")

            var rootScriptDirectory = path.dirname(require.main.filename)

            // Prompt password to create the DID
            console.log("Please set a password to protext your DID signature. Don't forget it, it can't be retrieved.")

            let typedInfo;
            do {
                const questions = [
                    {
                        type: 'password',
                        name: 'password',
                        message: 'DID signature private key password. 8 characters min:',
                        validate: value => {
                            return value != "" && value.length >= 8
                        }
                    },
                    {
                        type: 'password',
                        name: 'passwordRepeat',
                        message: 'Please type your password again:',
                        validate: value => {
                            return value != "" && value.length >= 8
                        }
                    },
                ];
                typedInfo = await prompts(questions);

                if (typedInfo.password != typedInfo.passwordRepeat) {
                    console.log("Sorry, password don't match, please try again.".red)
                }
            }
            while (typedInfo.password != typedInfo.passwordRepeat)
            console.log("")

            // TMP BEN: Mnemonic: stuff silent betray cherry balcony humor trip spy power pool behind lawn

            const spawn = require("child_process").spawn;
            const pythonProcess = spawn('python',[rootScriptDirectory+"/toolchain/create_did","-r","appdid","-m","stuff silent betray cherry balcony humor trip spy power pool behind lawn","-p",typedInfo.password,"-s",typedInfo.password]);

            pythonProcess.stdout.on('data', function (data) { console.log(''+data)});
            pythonProcess.stderr.on('data', function (data) { console.log(''+data)});
            pythonProcess.on('error', function(err) { reject(err)})

            pythonProcess.on('exit', function (code) {
                if (code == 0) {
                    // Successfully created the DID
                    console.log("DID created successfully locally on your computer".green)
                    resolve()
                }
                else {
                    reject('Child process exited with code ' + code)
                }
            });
        })
    }

    /**
     * After a DID is created, a DID request JSON structure has to be created. That request packages
     * the signed and base58 (?) encoded DID document of the created DID, into a CREATE DID request that 
     * can be stored on chain.
     */
    createDIDRequest() {
        return new Promise((resolve, reject)=>{
            // TMP
            resolve({
                "header":{
                    "specification":"elastos/did/1.0",
                    "operation":"create"
                },
                "payload":"eyJpZCI6ImRpZDplbGFzdG9zOmlZY3A3SkRCenhTZnFlcVV0VlQ1TG5yZ1dvNDhpUVV0Q2oiLCJwdWJsaWNLZXkiOlt7ImlkIjoiI3ByaW1hcnkiLCJwdWJsaWNLZXlCYXNlNTgiOiJ6bkduc042N3BFUXBwQ3FIS2t4TDJuNzV4MnlqSmNtcllrbW1MdnNoNGZSQSJ9XSwiYXV0aGVudGljYXRpb24iOlsiI3ByaW1hcnkiXSwiZXhwaXJlcyI6IjIwMjQtMTEtMTJUMTM6MDA6MDBaIn0",
                "proof":{
                    "verificationMethod":"#primary",
                    "signature":"h3PQyLMVR+vWXF6jPGmHSXDD/3QwjtBy17aqZ9DErL+2xNUE9s1NdSQ5jpBUAqXrG/8nGkBDVDYTHixV2uvBSw=="
                }
            }) 
        })     
    }

    /**
     * Generates a intent url that can be opened in trinity, to let the wallet application pay for 
     * a DID document creation transaction. This transaction payload is signed locally by the DID.
     */
    generateCreateDIDDocumentTransactionURL(didRequest) {
        return new Promise((resolve, reject)=>{
            jwt.sign(didRequest, "nosecretkey", { algorithm: 'none' }, (err, encodedJWT)=>{
                let url = "elastos://didtransaction/"+encodedJWT
                resolve(url)
            })
        })
    }

    /**
     * Generates a temporary local web page that can be opened on the computer in order to display
     * a QR code. This QR code should be scanned from the Trinity application in order to run the 
     * wallet app to pay DID transaction fees. 
     */
    generatePayForTransactionQRCodeWebPage(schemeUrl) {
        console.log("Creating a temporary web page to display a QR code...")
        return new Promise((resolve, reject)=>{
            let webpagePath = os.tmpdir() + "/publishdid.html"
            QRCode.toDataURL(schemeUrl,  async (err, imageDataUrl) => {
                let htmlData = "<html><body style='font-family:verdana'><center>";
                htmlData += "<h2>Please scan this QR code using Trinity from your mobile phone</h2>";
                htmlData += "<h3>You will be prompted to confirm publication of your DID on the DID sidechain</h3>";
                htmlData += "<img src='"+imageDataUrl+"' height='600'/>";
                htmlData += "</center></body></html>"

                fs.writeFileSync(webpagePath, htmlData)
    
                console.log("Launching your browser to display a QR code.");
                console.log("If this doesn't open automatically, please manually open ["+webpagePath+"].");
                
                await open("file://"+webpagePath);

                resolve(webpagePath)
            })
        })
    }

    /**
     * Infinite polling to check when the DID transaction has been written on the sidechain.
     * A DApp cannot be published on the DApp store without a valid DID on the sidechain so after
     * the DID is uploaded, we check the sidechain until we can see it appear. At that time we can
     * publish.
     */
    async waitForSidechainTransactionCompleted() {
        console.log("")
        console.log("Waiting for your DID to be ready on the DID sidechain. This could take several minutes.")
        console.log("Please now scan the QR code and validate the transaction from your trinity application.".magenta)
        console.log("")

        this.createdDIDFoundOnSidechain = false;
        // Debug: valid ID already on sidechain: iVPadJq56wSRDvtD5HKvCPNryHMk3qVSU4
        // Debug: valid ID not yet on sidechain: ihQrudV8ya5MfZRZW98dbiet1n1QCVAWPL
        this.targetDIDUrl = "ihQrudV8ya5MfZRZW98dbiet1n1QCVAWPL" // TMP TODO - SHould get this from the created DID
        this.didCreationCheckRetryCount = 0
        this.didCreationSpinnerMessage = "Starting"

        let self = this
        this.checkDIDCreationSpinner = new Spinner({
            text: '%s',
            stream: process.stdout,
            onTick: function(msg){
                this.clearLine(this.stream);
                this.stream.write(self.didCreationSpinnerMessage + " " + msg);
            }
        })
        this.checkDIDCreationSpinner.start();

        do {
            await this._checkDIDPresenceOnSidechain()
            await sleep(3000)
        }
        while (!this.createdDIDFoundOnSidechain)

        console.log("DONE! Your DID is now available on the DID sidechain.".green)
    }

    _stopCheckingDIDCreated() {
        this.checkDIDCreationSpinner.stop()
    }

    /**
     * Checks that a given DID exists on the DID sidechain using a centralized RPC API
     */
    async _checkDIDPresenceOnSidechain() {
        return new Promise(async (resolve, reject)=>{
            try {
                this.didCreationCheckRetryCount++
                this.didCreationSpinnerMessage = "Querying DID sidechain... ("+this.didCreationCheckRetryCount+")"

                let response = await axios({
                    method:"post",
                    url: "https://coreservices-didsidechain-privnet.elastos.org",     // Use this.targetDIDUrl in params
                    data: {
                        "method": "getidtxspayloads",
                        "params":{
                            "id": this.targetDIDUrl,
                            "all": false // Only the latest entry
                        }
                    },
                    headers: {
                        'Content-Type': 'application/json'
                    }
                })

                if (!response || !response.data) {
                    console.error("Failed to get response from the RPC API...")
                    console.log(response)
                }
                else {
                    if (response.data.result) {
                        console.log("found")
                        this.createdDIDFoundOnSidechain = true;    
                        this._stopCheckingDIDCreated()  
                    }
                }
            }
            catch (e) {
                //console.log(e.response)
                //console.error(e)
            }

            resolve()
        })
    }
}