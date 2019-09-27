import Foundation

public class VerifiableCredential: DIDObject {
    public var types: Array<String>!
    public var issuer: DID!
    public var issuanceDate: Date!
    public var expiationDate: Date!
    public var subject: CredentialSubject!
    public var proof: Proof!
    
    init(_ vc: VerifiableCredential) {
        super.init(vc.id, vc.type)
        self.id = vc.id
        self.types = vc.types
        self.issuer = vc.issuer
        self.issuanceDate = vc.issuanceDate
        self.expiationDate = vc.expiationDate
        self.subject = vc.subject
        self.proof = vc.proof
    }
    
    public func toJson(_ ref: DID, _ compact: Bool) -> Dictionary<String, Any> {
        var dic: Dictionary<String, Any> = [: ]
        var value: String
        
        // id
        if compact && id.isEqual(ref) {
            value = "#" + id.fragment
        }
        else {
            value = id.toExternalForm()
        }
        dic[Constants.id] = value
        
        // type
        var strs: Array<String> = []
        types.forEach{ str in
            strs.append(str)
        }
        dic[Constants.type] = strs
        
        // issuer
        if !compact && !(issuer.isEqual(subject.id)) {
            dic[Constants.issuer] = issuer.toExternalForm()
        }
        
        // issuanceDate
        if (expiationDate != nil) {
            dic[Constants.expirationDate] = "TODO: change to time string"
        }
        
        // credentialSubject
        dic[Constants.credentialSubject] = " TODO: "
        
        // proof
        // TODO: judge is sigin
        dic[Constants.proof] = "TODO: "
        return dic
    }
}
