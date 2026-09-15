#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

using namespace std;

struct Node {
    string rule;
    vector<string> tokens;
    vector<Node*> children;
    string type = "";
};

struct ProcedureInfo {
    vector<string> paramTypes;
    map<string, string> symbolTable;
    string returnType;
};

void deleteTree(Node* root);

vector<string> split(const string& line) {
    vector<string> tokens;
    stringstream ss(line);
    string token;

    while (ss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

bool analyzeTest(
    Node* testNode,
    ProcedureInfo& currentProc,
    map<string, ProcedureInfo>& procedures
);

bool analyzeStatement(
    Node* statementNode,
    ProcedureInfo& currentProc,
    map<string, ProcedureInfo>& procedures
);

bool analyzeStatements(
    Node* statementsNode,
    ProcedureInfo& currentProc,
    map<string, ProcedureInfo>& procedures
);

bool isTerminal(const string& s) {
    for (char c : s) {
        if (!isupper(c)) {
            return false;
        }
    }

    return true;
}

Node* buildTree() {
    string line;
    if (!getline(cin, line) || line.empty()) {
        return nullptr;
    }

    Node* node = new Node;
    node->rule = line;
    node->tokens = split(line);

    if (node->tokens.empty()) {
        delete node;
        return nullptr;
    }

    string lhs = node->tokens[0];

    if (isTerminal(lhs)) {
        return node;
    }

    if (node->tokens.size() == 2 && node->tokens[1] == ".EMPTY") {
        return node;
    }

    for (int i = 1; i < (int)node->tokens.size(); ++i) {
        Node* child = buildTree();
        if (!child) {
            deleteTree(node);
            return nullptr;
        }
        node->children.push_back(child);
    }

    return node;
}

void printTree(const Node* root) {
    if (!root) return;

    cout << root->rule;

    if (root->type != "") {
        cout << " : " << root->type;
    }

    cout << endl;

    for (Node* child : root->children) {
        printTree(child);
    }
}

void deleteTree(Node* root) {
    if (!root) return;

    for (Node* child : root->children) {
        deleteTree(child);
    }

    delete root;
}

string getDclType(Node* dcl) {
    Node* typeNode = dcl->children[0];

    if (typeNode->rule == "type LONG") {
        return "long";
    }

    if (typeNode->rule == "type LONG STAR") {
        return "long*";
    }

    return "ERROR";
}

string getDclName(Node* dcl) {
    return dcl->children[1]->tokens[1];
}

//add checks and builds out dcls children and proc data 
bool addDcl(Node* dcl, ProcedureInfo& proc) {
    string name = getDclName(dcl);
    string type = getDclType(dcl);

    if (type == "ERROR") {
        return false;
    }

    if (proc.symbolTable.count(name)) {
        return false;
    }

    proc.symbolTable[name] = type;

    // Annotate the ID inside the declaration
    dcl->children[1]->type = type;

    return true;
}

string initType(Node* initNode) {
    if (initNode->tokens[0] == "NUM") {
        initNode->type = "long";
        return "long";
    }

    if (initNode->tokens[0] == "NULL") {
        initNode->type = "long*";
        return "long*";
    }

    return "ERROR";
}

bool analyzeDcls(Node* dclsNode, ProcedureInfo& proc) {
    if (!dclsNode) {
        return true;
    }

    // dcls .EMPTY
    if (dclsNode->rule == "dcls .EMPTY") {
        return true;
    }

    // dcls dcls dcl BECOMES NUM SEMI
    // dcls dcls dcl BECOMES NULL SEMI
    if (dclsNode->rule == "dcls dcls dcl BECOMES NUM SEMI" ||
        dclsNode->rule == "dcls dcls dcl BECOMES NULL SEMI") {

        Node* previousDcls = dclsNode->children[0];
        Node* dcl = dclsNode->children[1];
        Node* initializer = dclsNode->children[3];

        // analyze earlier declarations first
        if (!analyzeDcls(previousDcls, proc)) {
            return false;
        }

        string declaredType = getDclType(dcl);
        string initializerType = initType(initializer);

        if (declaredType != initializerType) {
            return false;
        }

        if (!addDcl(dcl, proc)) {
            return false;
        }

        return true;
    }

    return false;
}

string typeExpr(Node* root, ProcedureInfo& currentProc, map<string, ProcedureInfo>& procedures);

string typeLvalue(Node* root, ProcedureInfo& proc, map<string, ProcedureInfo>& procedures) {
    if (!root) { return "ERROR"; }

    // lvalue ID
    if (root->rule == "lvalue ID") {
        Node* idNode = root->children[0];
        string name = idNode->tokens[1];

        if (!proc.symbolTable.count(name)) {
            return "ERROR";
        }

        string t = proc.symbolTable[name];

        idNode->type = t;
        root->type = t;
        return t;
    }

    // lvalue STAR factor
    if (root->rule == "lvalue STAR factor") {
        string factorType = typeExpr(root->children[1], proc, procedures);

        if (factorType != "long*") {
            return "ERROR";
        }

        root->type = "long";
        return "long";
    }

    // lvalue LPAREN lvalue RPAREN
    if (root->rule == "lvalue LPAREN lvalue RPAREN") {
        string t = typeLvalue(root->children[1], proc, procedures);

        if (t == "ERROR") {
            return "ERROR";
        }

        root->type = t;
        return t;
    }

    return "ERROR";

}

bool collectArgTypes(
    Node* arglist,
    ProcedureInfo& currentProc,
    map<string, ProcedureInfo>& procedures,
    vector<string>& argTypes
) {
    if (arglist->rule == "arglist expr") {
        string t = typeExpr(
            arglist->children[0],
            currentProc,
            procedures
        );

        if (t == "ERROR") {
            return false;
        }

        argTypes.push_back(t);
        return true;
    }

    if (arglist->rule == "arglist expr COMMA arglist") {
        string t = typeExpr(
            arglist->children[0],
            currentProc,
            procedures
        );

        if (t == "ERROR") {
            return false;
        }

        argTypes.push_back(t);

        return collectArgTypes(
            arglist->children[2],
            currentProc,
            procedures,
            argTypes
        );
    }

    return false;
}
 
string typeExpr(Node* root, ProcedureInfo& currentProc, map<string, ProcedureInfo>& procedures) {
    if (!root) return "ERROR";

    if (root->rule == "factor STAR factor") {
        string factorType = typeExpr(root->children[1], currentProc, procedures);

        if (factorType != "long*") {
            return "ERROR";
        }

        root->type = "long";
        return "long";
    }

    if (root->rule == "factor AMP lvalue") {
    string lvalueType = typeLvalue(root->children[1], currentProc, procedures);

        if (lvalueType != "long") {
            return "ERROR";
        }

        root->type = "long*";
        return "long*";
    }

    if (root->rule == "factor NEW LONG LBRACK expr RBRACK") {
        string exprType = typeExpr(root->children[3], currentProc, procedures);

        if (exprType != "long") {
            return "ERROR";
        }

        root->type = "long*";
        return "long*";
    }

    if (root->rule == "expr term") {
        string t = typeExpr(root->children[0], currentProc, procedures);

        if (t == "ERROR") {
            return "ERROR";
        }

        root->type = t;
        return t;
    }

    if (root->rule == "term term STAR factor" ||
        root->rule == "term term SLASH factor" ||
        root->rule == "term term PCT factor") {

        string leftType = typeExpr(root->children[0], currentProc, procedures);
        string rightType = typeExpr(root->children[2], currentProc, procedures);

        if (leftType != "long" || rightType != "long") {
            return "ERROR";
        }

        root->type = "long";
        return "long";
    }

    if (root->rule == "term factor") {
        string t = typeExpr(root->children[0], currentProc, procedures);

        if (t == "ERROR") {
            return "ERROR";
        }

        root->type = t;
        return t;
    }

if (root->rule == "expr expr PLUS term") {
    string leftType =
        typeExpr(root->children[0], currentProc, procedures);

    string rightType =
        typeExpr(root->children[2], currentProc, procedures);

    if (leftType == "ERROR" || rightType == "ERROR") {
        return "ERROR";
    }

    if (leftType == "long" && rightType == "long") {
        root->type = "long";
        return "long";
    }

    if (leftType == "long*" && rightType == "long") {
        root->type = "long*";
        return "long*";
    }

    if (leftType == "long" && rightType == "long*") {
        root->type = "long*";
        return "long*";
    }

    return "ERROR";
}

if (root->rule == "expr expr MINUS term") {
    string leftType =
        typeExpr(root->children[0], currentProc, procedures);

    string rightType =
        typeExpr(root->children[2], currentProc, procedures);

    if (leftType == "ERROR" || rightType == "ERROR") {
        return "ERROR";
    }

    if (leftType == "long" && rightType == "long") {
        root->type = "long";
        return "long";
    }

    if (leftType == "long*" && rightType == "long") {
        root->type = "long*";
        return "long*";
    }

    if (leftType == "long*" && rightType == "long*") {
        root->type = "long";
        return "long";
    }

    return "ERROR";
}

if (root->rule == "factor ID LPAREN RPAREN") {
    Node* idNode = root->children[0];
    string name = idNode->tokens[1];

    // A variable cannot be called like a procedure.
    if (currentProc.symbolTable.count(name)) {
        return "ERROR";
    }

    // Procedure must already have been declared.
    if (!procedures.count(name)) {
        return "ERROR";
    }

    // This call has no arguments, so the procedure must expect none.
    if (!procedures.at(name).paramTypes.empty()) {
        return "ERROR";
    }

    // Procedure-name ID terminals are not annotated.
    root->type = "long";
    return "long";
}

if (root->rule == "factor ID LPAREN arglist RPAREN") {
    Node* idNode = root->children[0];
    string name = idNode->tokens[1];

    // A variable cannot be called like a procedure.
    if (currentProc.symbolTable.count(name)) {
        return "ERROR";
    }

    // Procedure must already have been declared.
    if (!procedures.count(name)) {
        return "ERROR";
    }

    vector<string> argTypes;

    if (!collectArgTypes(
            root->children[2],
            currentProc,
            procedures,
            argTypes)) {
        return "ERROR";
    }

    if (argTypes != procedures.at(name).paramTypes) {
        return "ERROR";
    }

    // Every WLP4 procedure returns long.
    root->type = "long";
    return "long";
}

if (root->rule == "factor GETCHAR LPAREN RPAREN") {
    root->type = "long";
    return "long";
}

if (root->rule == "factor NUM") {
    string t =
        typeExpr(root->children[0], currentProc, procedures);

    if (t == "ERROR") {
        return "ERROR";
    }

    root->type = t;
    return t;
}

if (root->rule == "factor NULL") {
    string t =
        typeExpr(root->children[0], currentProc, procedures);

    if (t == "ERROR") {
        return "ERROR";
    }

    root->type = t;
    return t;
}

if (root->rule == "factor ID") {
    Node* idNode = root->children[0];
    string name = idNode->tokens[1];

    if (!currentProc.symbolTable.count(name)) {
        return "ERROR";
    }

    string t = currentProc.symbolTable.at(name);

    idNode->type = t;
    root->type = t;

    return t;
}

if (root->rule == "factor LPAREN expr RPAREN") {
    string t =
        typeExpr(root->children[1], currentProc, procedures);

    if (t == "ERROR") {
        return "ERROR";
    }

    root->type = t;
    return t;
}

if (root->tokens[0] == "NUM") {
    root->type = "long";
    return "long";
}

if (root->tokens[0] == "NULL") {
    root->type = "long*";
    return "long*";
}

return "ERROR";
}

bool analyzeTest(
    Node* testNode,
    ProcedureInfo& currentProc,
    map<string, ProcedureInfo>& procedures
) {
    if (!testNode) {
        return false;
    }

    if (testNode->rule == "test expr EQ expr" ||
        testNode->rule == "test expr NE expr" ||
        testNode->rule == "test expr LT expr" ||
        testNode->rule == "test expr LE expr" ||
        testNode->rule == "test expr GE expr" ||
        testNode->rule == "test expr GT expr") {

        string leftType = typeExpr(
            testNode->children[0],
            currentProc,
            procedures
        );

        string rightType = typeExpr(
            testNode->children[2],
            currentProc,
            procedures
        );

        if (leftType == "ERROR" || rightType == "ERROR") {
            return false;
        }

        if (leftType != rightType) {
            return false;
        }

        return true;
    }

    return false;
}

bool analyzeStatements(Node* statementsNode, ProcedureInfo& currentProc, map<string, ProcedureInfo>& procedures) {
    if (!statementsNode) {
        return false;
    }

    if (statementsNode->rule == "statements .EMPTY") {
        return true;
    }

    if (statementsNode->rule ==
        "statements statements statement") {

        Node* previousStatements = statementsNode->children[0];
        Node* currentStatement = statementsNode->children[1];

        if (!analyzeStatements(
                previousStatements,
                currentProc,
                procedures)) {
            return false;
        }

        return analyzeStatement(
            currentStatement,
            currentProc,
            procedures
        );
    }

    return false;
}

bool analyzeStatement(Node* statementNode, ProcedureInfo& currentProc, map<string, ProcedureInfo>& procedures) {
    if (!statementNode) {
        return false;
    }

    // statement lvalue BECOMES expr SEMI
    if (statementNode->rule ==
        "statement lvalue BECOMES expr SEMI") {

        string leftType = typeLvalue(
            statementNode->children[0],
            currentProc,
            procedures
        );

        string rightType = typeExpr(
            statementNode->children[2],
            currentProc,
            procedures
        );

        if (leftType == "ERROR" || rightType == "ERROR") {
            return false;
        }

        // Assignment requires matching types.
        return leftType == rightType;
    }

    // statement PRINTLN LPAREN expr RPAREN SEMI
    if (statementNode->rule ==
        "statement PRINTLN LPAREN expr RPAREN SEMI") {

        string exprType = typeExpr(
            statementNode->children[2],
            currentProc,
            procedures
        );

        return exprType == "long";
    }

    // statement PUTCHAR LPAREN expr RPAREN SEMI
    if (statementNode->rule ==
        "statement PUTCHAR LPAREN expr RPAREN SEMI") {

        string exprType = typeExpr(
            statementNode->children[2],
            currentProc,
            procedures
        );

        return exprType == "long";
    }

    // statement DELETE LBRACK RBRACK expr SEMI
    if (statementNode->rule ==
        "statement DELETE LBRACK RBRACK expr SEMI") {

        string exprType = typeExpr(
            statementNode->children[3],
            currentProc,
            procedures
        );

        return exprType == "long*";
    }

    // statement WHILE LPAREN test RPAREN
    //           LBRACE statements RBRACE
    if (statementNode->rule ==
        "statement WHILE LPAREN test RPAREN LBRACE statements RBRACE") {

        Node* testNode = statementNode->children[2];
        Node* bodyStatements = statementNode->children[5];

        if (!analyzeTest(
                testNode,
                currentProc,
                procedures)) {
            return false;
        }

        return analyzeStatements(
            bodyStatements,
            currentProc,
            procedures
        );
    }

    // statement IF LPAREN test RPAREN LBRACE statements RBRACE
    //           ELSE LBRACE statements RBRACE
    if (statementNode->rule ==
        "statement IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE") {

        Node* testNode = statementNode->children[2];
        Node* ifStatements = statementNode->children[5];
        Node* elseStatements = statementNode->children[9];

        if (!analyzeTest(
                testNode,
                currentProc,
                procedures)) {
            return false;
        }

        if (!analyzeStatements(
                ifStatements,
                currentProc,
                procedures)) {
            return false;
        }

        if (!analyzeStatements(
                elseStatements,
                currentProc,
                procedures)) {
            return false;
        }

        return true;
    }

    return false;
}



bool analyzeMain(Node* mainNode, map<string, ProcedureInfo>& procedures) {
    // main LONG WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
    
    ProcedureInfo wain;
    
    Node* firstParam = mainNode->children[3]; // dcl 1
    Node* secondParam = mainNode->children[5]; // dcl 2
    Node* dclsNode = mainNode->children[8]; // internal dcl 
    Node* statementsNode = mainNode->children[9];
    Node* returnExpr = mainNode->children[11]; // return expr

    //params added before decls 
    if (!addDcl(firstParam, wain)) {
        return false;
    }

    wain.paramTypes.push_back(getDclType(firstParam));

    if (!addDcl(secondParam, wain)) {
        return false;
    }

    wain.paramTypes.push_back(getDclType(secondParam));

    //second parameter of wain must be long
    if (wain.paramTypes[1] != "long") {
        return false;
    }

    //local variables after the parameters
    if (!analyzeDcls(dclsNode, wain)) {
        return false;
    }

    //statements can now use parameters and locals
    if (!analyzeStatements(
            statementsNode,
            wain,
            procedures)) {
        return false;
    }

    //check return expression
    wain.returnType = typeExpr(
        returnExpr,
        wain,
        procedures
    );

    if (wain.returnType != "long") {
        return false;
    }

    procedures["wain"] = wain;

    return true;
}

bool analyzeParamlist(Node* node, ProcedureInfo& proc) {
    if (node->rule == "paramlist dcl") {
        Node* dcl = node->children[0];

        if (!addDcl(dcl, proc)) {
            return false;
        }

        proc.paramTypes.push_back(getDclType(dcl));
        return true;
    }

    if (node->rule == "paramlist dcl COMMA paramlist") {
        Node* dcl = node->children[0];
        Node* rest = node->children[2];

        if (!addDcl(dcl, proc)) {
            return false;
        }

        proc.paramTypes.push_back(getDclType(dcl));

        return analyzeParamlist(rest, proc);
    }

    return false;
}

bool analyzeParams(Node* params, ProcedureInfo& proc) {
    if (params->rule == "params .EMPTY") {
        return true;
    }

    if (params->rule == "params paramlist") {
        return analyzeParamlist(params->children[0], proc);
    }

    return false;
}


bool analyzeProcedure(
    Node* procedureNode,
    map<string, ProcedureInfo>& procedures
) {
    string name = procedureNode->children[1]->tokens[1];

    if (procedures.count(name)) {
        return false;
    }

    ProcedureInfo proc;

    Node* paramsNode = procedureNode->children[3];
    Node* dclsNode = procedureNode->children[6];
    Node* statementsNode = procedureNode->children[7];
    Node* returnExpr = procedureNode->children[9];

    if (!analyzeParams(paramsNode, proc)) {
        return false;
    }

    // Make signature visible for recursive calls.
    procedures[name] = proc;

    if (!analyzeDcls(dclsNode, procedures[name])) {
        return false;
    }

    if (!analyzeStatements(
            statementsNode,
            procedures[name],
            procedures)) {
        return false;
    }

    string returnType = typeExpr(
        returnExpr,
        procedures[name],
        procedures
    );

    if (returnType != "long") {
        return false;
    }

    procedures[name].returnType = returnType;

    return true;
}

bool analyzeProcedures(Node* node, map<string, ProcedureInfo>& procedures) {

    if (node->rule == "procedures main") {

        return analyzeMain(node->children[0], procedures);
    }

    if (node->rule == "procedures procedure procedures") {
        Node* procedureNode = node->children[0];
        Node* remainingProcedures = node->children[1];

        if (!analyzeProcedure(procedureNode, procedures)) {
            return false;
        }

        return analyzeProcedures(remainingProcedures, procedures);
    }

    return false;
}

int main() {
    Node* root = buildTree();

    if (!root) {
        cerr << "ERROR: empty or truncated parse tree" << endl;
        return 1;
    }

    map<string, ProcedureInfo> procedures;

    Node* proceduresRoot = root;
    if (root->rule == "start BOF procedures EOF" && root->children.size() == 3) {
        proceduresRoot = root->children[1];
    }

    if (!analyzeProcedures(proceduresRoot, procedures)) {
        cerr << "ERROR" << endl;
        deleteTree(root);
        return 1;
    }

    printTree(root);

    deleteTree(root);
    return 0;
}
