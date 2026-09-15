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

struct SymbolTable {
    map<string, pair<string, int>> table;
    string prod; 
};


//All the prods with name (key) and thier symbol table (value)
struct ProdTables {
    map<string, SymbolTable> prods;

};


int labelCounter = 0; // For keeping track of my labels

void deleteTree(Node* tree);

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

//SPlit the line up into an array of tokens 
vector<string> split(const string& line) {
    vector<string> tokens;
    stringstream ss(line);
    string token; 

    while(ss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}


bool isTerminal(string token) {
    for (char c : token) {
        if (!isupper(c)) {
            return false;
        }
    }

    return true;
}


Node* buildTree() {
    string line;

    if (!getline(cin, line)) {
        return nullptr;
    }

    Node* node = new Node;
    node->tokens = split(line);

    if (node->tokens.empty()) {
        delete node;
        return nullptr;
    }

    // Find where the actual grammar rule ends
    int ruleEnd = node->tokens.size();

    for (int i = 0; i < (int)node->tokens.size(); ++i) {
        if (node->tokens[i] == ":") {
            node->type = node->tokens[i + 1];
            ruleEnd = i;
            break;
        }
    }

    // Store the rule without the type annotation
    node->rule = node->tokens[0];

    for (int i = 1; i < ruleEnd; ++i) {
        node->rule += " " + node->tokens[i];
    }

    string lhs = node->tokens[0];

    // Terminals never have children
    if (isTerminal(lhs)) {
        return node;
    }

    // Empty productions have no children
    if (ruleEnd == 2 && node->tokens[1] == ".EMPTY") {
        return node;
    }

    // Only build children for symbols before ":"
    for (int i = 1; i < ruleEnd; ++i) {
        Node* child = buildTree();

        if (child == nullptr) {
            cerr << "Unexpected end of input" << endl;
            deleteTree(node);
            exit(1);
        }

        node->children.push_back(child);
    }

    return node;
}

void pushX0() {
    cout << "stur x0, [sp, -8]" << endl;
    cout << "sub sp, sp, x8" << endl;
}

void popX1() {
    cout << "add sp, sp, x8" << endl;
    cout << "ldur x1, [sp, -8]" << endl;
}

void pushX30() {
    cout << "stur x30, [sp, -8]" << endl;
    cout << "sub sp, sp, x8" << endl;
}

void popX30() {
    cout << "add sp, sp, x8" << endl;
    cout << "ldur x30, [sp, -8]" << endl;
}

void loadAndSkip(
    const string& reg,
    const string& value) {

    cout << "ldr " << reg << ", 8" << endl;
    cout << "b 12" << endl;
    cout << ".8byte " << value << endl;
}


void deleteTree(Node* root) {
    if (!root) return;

    for (Node* child : root->children) {
        deleteTree(child);
    }

    delete root;
}

void generateStatements(
    Node* root,
    const SymbolTable& symbolTable);


void generateDcls(
    Node* root,
    SymbolTable& symbolTable, int& nextOffset) {

        // dcls -> .EMPTY
        if (root->rule == "dcls .EMPTY") {
            return;
        }

        //dcls -> dcls dcl BECOMES NUM SEMI

        // get rid of that first dcls fuh dat bug
        generateDcls(root->children[0], symbolTable, nextOffset); 

        //dcl -> type ID

        Node* DCLNode = root->children[1];
        Node* IDNode = DCLNode->children[1];

        //Get all the info 
        string type = IDNode->type;
        string name = IDNode->tokens[1];

        if (root->rule == "dcls dcls dcl BECOMES NUM SEMI") {
            Node* numNode = root->children[3];
            string value = numNode->tokens[1];


            // Load the initial value into x0
            cout << "ldr x0, 8" << endl;
            cout << "b 12" << endl;
            cout << ".8byte " << value << endl;

            // Push the local variable onto the stack
            cout << "stur x0, [sp, -8]" << endl;
            cout << "sub sp, sp, x8" << endl;

            //Loading into symbolTabel
            symbolTable.table[name] = {type, nextOffset};
            nextOffset -= 8;
        } else if (root->rule == "dcls dcls dcl BECOMES NULL SEMI") {

            loadAndSkip("x0", "0xffffffffffff0000"); //Load on NULL

            pushX0(); // push null onto the stack

            symbolTable.table[name] = {type, nextOffset}; // store into the table

            nextOffset -= 8;
            return;

        }
    }


void generateExpr(
    Node* root,
    const SymbolTable& symbolTable);

void generateFactor(
    Node* root,
    const SymbolTable& symbolTable);


void storeLvalue(
    Node* root,
    const SymbolTable& symbolTable) {

    // lvalue -> ID
    if (root->rule == "lvalue ID") {
        Node* idNode = root->children[0];
        string name = idNode->tokens[1];

        int offset = symbolTable.table.at(name).second;

        // x0 contains the new value
        cout << "stur x0, [x29, "
             << offset << "]" << endl;
    }

    // lvalue -> LPAREN lvalue RPAREN
    else if (root->rule == "lvalue LPAREN lvalue RPAREN") {
        storeLvalue(root->children[1], symbolTable);
    }

    else if (root->rule == "lvalue STAR factor") {
        // Save the right-hand value
        pushX0();

        // Generate the pointer address into x0
        generateFactor(
            root->children[1],
            symbolTable
        );

        // Restore right hand value into x1
        popX1();

        // Store the value in x1 at the address in x0
        cout << "stur x1, [x0, 0]" << endl;
    }
}

void generateTest(
    Node* root,
    const SymbolTable& symbolTable,
    const string& falseLabel) {

    // Generate left expression into x0
    generateExpr(root->children[0], symbolTable);

    // Save left expression
    pushX0();

    // Generate right expression into x0
    generateExpr(root->children[2], symbolTable);

    // Restore left expression into x1
    popX1();

    bool pointerComparison =
    root->children[0]->type == "long*";

    // x1 = left and x0 = right
    cout << "cmp x1, x0" << endl;

    // test -> expr EQ expr
    if (root->rule == "test expr EQ expr") {
        cout << "b.ne " << falseLabel << endl;
    }

    //false when left == right
    else if (root->rule == "test expr NE expr") {
        cout << "b.eq " << falseLabel << endl;
    } 

    else if (pointerComparison) {
        // Unsigned pointer comparisons
        if (root->rule == "test expr LT expr") {
            cout << "b.hs " << falseLabel << endl;
        }

        else if (root->rule == "test expr LE expr") {
            cout << "b.hi " << falseLabel << endl;
        }

        else if (root->rule == "test expr GE expr") {
            cout << "b.lo " << falseLabel << endl;
        }

        else if (root->rule == "test expr GT expr") {
            cout << "b.ls " << falseLabel << endl;
        }
    } else {

        // false when left >= right
        if (root->rule == "test expr LT expr") {
            cout << "b.ge " << falseLabel << endl;
        }

        // false when left > right
        else if (root->rule == "test expr LE expr") {
            cout << "b.gt " << falseLabel << endl;
        }

        // false when left < right
        else if (root->rule == "test expr GE expr") {
            cout << "b.lt " << falseLabel << endl;
        }

        // false when left <= right
        else if (root->rule == "test expr GT expr") {
            cout << "b.le " << falseLabel << endl;
        }
    }
}

void generateStatement(
    Node* root,
    const SymbolTable& symbolTable) {

    // statement → IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE
    if (root->rule ==
        "statement IF LPAREN test RPAREN LBRACE "
        "statements RBRACE ELSE LBRACE statements RBRACE") {
        
        //I need them damn else and if counters 
        int id = labelCounter++;

        string elseLabel = "else" + to_string(id);
        string endLabel = "endif" + to_string(id);

        Node* testNode = root->children[2];
        Node* thenStatements = root->children[5];
        Node* elseStatements = root->children[9];

        // If the condition is false, jump to the else section
        generateTest(testNode, symbolTable, elseLabel);

        //Generate statements inside the if body
        generateStatements(thenStatements, symbolTable);
        
        // Do not execute the else body after the if body
        cout << "b " << endLabel << endl;

        // Beginning of else
        cout << elseLabel << ":" << endl;

        generateStatements(elseStatements, symbolTable);

        //end of the entire if else
        cout << endLabel << ":" << endl;

        return;
    }

    if (root->rule ==
        "statement WHILE LPAREN test RPAREN "
        "LBRACE statements RBRACE") {

        int id = labelCounter++;

        string loopLabel = "loop" + to_string(id);
        string endLabel = "endloop" + to_string(id);

        Node* testNode = root->children[2];
        Node* bodyStatements = root->children[5];

        // Start loop
        cout << loopLabel << ":" << endl;

        // exit the loop
        generateTest(
            testNode,
            symbolTable,
            endLabel
        );

        generateStatements(
            bodyStatements,
            symbolTable
        );

        // Return to the start
        cout << "b " << loopLabel << endl;

        // when the test is false
        cout << endLabel << ":" << endl;

        return;
    }

    //Putchar case 
    if (root->rule == "statement PUTCHAR LPAREN expr RPAREN SEMI") {

        Node* exprNode = root->children[2];

        generateExpr(exprNode, symbolTable);

        loadAndSkip("x1", "0xc000000000010008");

        cout << "stur x0, [x1, 0]" << endl;

        return;
    }

    //Println case
    if (root->rule == "statement PRINTLN LPAREN expr RPAREN SEMI") {

        Node* exprNode = root->children[2];

        generateExpr(exprNode, symbolTable);

        pushX30();

        loadAndSkip("x1", "print");

        cout << "blr x1" << endl;

        popX30();

        return;
    }

    // statement -> lvalue BECOMES expr SEMI
    if (root->rule == "statement lvalue BECOMES expr SEMI") {

        Node* lvalueNode = root->children[0];
        Node* exprNode = root->children[2];

        // Generate the new value into x0
        generateExpr(exprNode, symbolTable);

        // Store x0 into the lvalue
        storeLvalue(lvalueNode, symbolTable);

        return;
    }

    if (root->rule == "statement DELETE LBRACK RBRACK expr SEMI") {
        generateExpr(
            root->children[3],
            symbolTable
        );

        int id = labelCounter++;
        string skipLabel =
            "skipDelete" + to_string(id);

        // Load NULL
        loadAndSkip(
            "x1",
            "0xffffffffffff0000"
        );

        cout << "cmp x0, x1" << endl;
        cout << "b.eq " << skipLabel << endl;

        pushX30();

        loadAndSkip("x1", "delete");
        cout << "blr x1" << endl;

        popX30();

        cout << skipLabel << ":" << endl;
        return;
    }
}

void generateStatements(
    Node* root,
    const SymbolTable& symbolTable) {

    // statements → .EMPTY
    if (root->rule == "statements .EMPTY") {
        return;
    }

    // statements → statements statement

    // Generate earlier statements first
    generateStatements(
        root->children[0],
        symbolTable
    );

    // Generate the current statement
    generateStatement(
        root->children[1],
        symbolTable
    );
}


int loadArgs(Node* node, const SymbolTable& symbolTable) {

    generateExpr(
        node->children[0],
        symbolTable
    );

    pushX0();

    if (node->rule == "arglist expr") {
        return 1;
    }

    // arglist → expr COMMA arglist
    int remaining = loadArgs(
        node->children[2],
        symbolTable
    );

    return 1 + remaining;

}

void generateFactor(
    Node* root,
    const SymbolTable& symbolTable
);

void generateAddress(Node* root, const SymbolTable& symbolTable) {

    if (root->rule == "lvalue ID") {
        string name =
            root->children[0]->tokens[1];

        int offset =
            symbolTable.table.at(name).second;

        loadAndSkip(
            "x1",
            to_string(offset)
        );

        cout << "add x0, x29, x1" << endl;
    } else if (
        root->rule == "lvalue LPAREN lvalue RPAREN"
    ) {
        generateAddress(
            root->children[1],
            symbolTable
        );
    } else if (root->rule == "lvalue STAR factor") {
        generateFactor(
            root->children[1],
            symbolTable
        );
    }

}



void generateFactor(
    Node* root,
    const SymbolTable& symbolTable) {

    // factor -> NUM
    if (root->rule == "factor NUM") {
        string value = root->children[0]->tokens[1];

        cout << "ldr x0, 8" << endl;
        cout << "b 12" << endl;
        cout << ".8byte " << value << endl;
    }

    // factor -> ID
    else if (root->rule == "factor ID") {
        string name = root->children[0]->tokens[1];

        int offset = symbolTable.table.at(name).second;

        cout << "ldur x0, [x29, " << offset << "]" << endl;
    }

    // factor -> LPAREN expr RPAREN
    else if (root->rule == "factor LPAREN expr RPAREN") {
        generateExpr(root->children[1], symbolTable);
    }

    // factor -> GETCHAR LPAREN RPAREN
    else if (root->rule == "factor GETCHAR LPAREN RPAREN") {

        loadAndSkip("x1", "0xc000000000010000");

        cout << "ldur x0, [x1, 0]" << endl;
    }

    //actor → ID LPAREN RPAREN
    else if (root->rule == "factor ID LPAREN RPAREN") {
        string label = root->children[0]->tokens[1];

        pushX30();

        loadAndSkip("x1", "F" + label);

        cout << "blr x1" << endl;

        popX30();

    }

    //factor -> ID LPAREN arglist RPAREN
    else if (root->rule == "factor ID LPAREN arglist RPAREN") {
        string label = root->children[0]->tokens[1];

        pushX30();

        int numArgs = loadArgs(root->children[2], symbolTable);

        loadAndSkip("x1", "F" + label);

        cout << "blr x1" << endl;

            // Remove arguments from the stack
    for (int i = 0; i < numArgs; ++i) {
        cout << "add sp, sp, x8" << endl;
    }

        popX30();

    } 
    
    else if (root->rule == "factor NULL") {
        loadAndSkip("x0", "0xffffffffffff0000"); // Load Null
    } 
    
    else if (root->rule == "factor STAR factor") {
        generateFactor(
            root->children[1],
            symbolTable
        );

        // x0 contains an address
        cout << "ldur x0, [x0, 0]" << endl;
    } 
    
    else if (root->rule == "factor AMP lvalue") {
        generateAddress(
            root->children[1],
            symbolTable
        );
    } 
    // factor -> NEW LONG LBRACK expr RBRACK
    else if (root->rule == "factor NEW LONG LBRACK expr RBRACK") {
        // x0 = tequested number of longs
        generateExpr(
            root->children[3],
            symbolTable
        );

        pushX30();

        loadAndSkip("x1", "new");
        cout << "blr x1" << endl;

        popX30();

        int id = labelCounter++;
        string successLabel =
            "newSuccess" + to_string(id);

        // Allocation library returns 0 on failure
        cout << "cmp x0, xzr" << endl;
        cout << "b.ne " << successLabel << endl;

        // Convert 0 to WLP4 NULL
        loadAndSkip(
            "x0",
            "0xffffffffffff0000"
        );

        cout << successLabel << ":" << endl;
    }


}

void generateTerm(
    Node* root,
    const SymbolTable& symbolTable) {

    // term -> factor
    if (root->rule == "term factor") {
        generateFactor(
            root->children[0],
            symbolTable
        );
    }

    // term -> term STAR factor
    else if (root->rule == "term term STAR factor") {
        generateTerm(
            root->children[0],
            symbolTable
        );

        pushX0();

        generateFactor(
            root->children[2],
            symbolTable
        );

        popX1();

        // x0 = left * right
        cout << "mul x0, x1, x0" << endl;
    }

    // term -> term SLASH factor
    else if (root->rule == "term term SLASH factor") {
        generateTerm(
            root->children[0],
            symbolTable
        );

        pushX0();

        generateFactor(
            root->children[2],
            symbolTable
        );

        popX1();

        // x0 = left / right
        cout << "sdiv x0, x1, x0" << endl;
    }

    // term → term PCT factor
    else if (root->rule == "term term PCT factor") {
        generateTerm(
            root->children[0],
            symbolTable
        );

        pushX0();

        generateFactor(
            root->children[2],
            symbolTable
        );

        popX1();

        /*
         * At this point:
         * x1 = left
         * x0 = right
         *
         * left % right =
         * left - (left / right) * right
         */

        // x2 = left / right
        cout << "sdiv x2, x1, x0" << endl;

        // x2 = (left / right) * right
        cout << "mul x2, x2, x0" << endl;

        // x0 = left - ((left / right) * right)
        cout << "sub x0, x1, x2" << endl;
    }
}


void generateExpr(
    Node* root,
    const SymbolTable& symbolTable) {

    // expr → term
    if (root->rule == "expr term") {
        generateTerm(
            root->children[0],
            symbolTable
        );
    }

    // expr → expr PLUS term
    else if (root->rule == "expr expr PLUS term") {
        string leftType = root->children[0]->type;
        string rightType = root->children[2]->type;


        // Generate left expression into x0
        generateExpr(
            root->children[0],
            symbolTable
        );

        // Save the left result
        pushX0();

        //generate right term into x0
        generateTerm(
            root->children[2],
            symbolTable
        );

        // Restore left result into x1
        popX1();

        // long + long
        if (leftType == "long" && rightType == "long") {
            cout << "add x0, x1, x0" << endl;
        }

        // long* + long
        else if (
            leftType == "long*" &&
            rightType == "long"
        ) {
            // Scale integer by 8 bytes
            cout << "mul x0, x0, x8" << endl;
            cout << "add x0, x1, x0" << endl;
        }

        // long + long*
        else {
            // Scale the integer on the left
            cout << "mul x1, x1, x8" << endl;
            cout << "add x0, x1, x0" << endl;
        }
    }

    // expr → expr MINUS term
    else if (root->rule == "expr expr MINUS term") {
        string leftType = root->children[0]->type;
        string rightType = root->children[2]->type;

        generateExpr(
            root->children[0],
            symbolTable
        );

        pushX0();

        generateTerm(
            root->children[2],
            symbolTable
        );

        popX1();

        // long - long
        if (leftType == "long" && rightType == "long") {
            cout << "sub x0, x1, x0" << endl;
        }

        // long* - long
        else if (
            leftType == "long*" &&
            rightType == "long"
        ) {
            cout << "mul x0, x0, x8" << endl;
            cout << "sub x0, x1, x0" << endl;
        }

        // long* - long*
        else {
            // First get byte difference
            cout << "sub x0, x1, x0" << endl;

            // Convert bytes to number of longs
            cout << "sdiv x0, x0, x8" << endl;
        }
    }
}



//LONG WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE
void generateWain(Node* root, SymbolTable& symbolTable) {
    Node* firstDcl = root->children[3];
    Node* secondDcl = root->children[5];
    Node* dcls = root->children[8];
    Node* statements = root->children[9];
    Node* returnExpr = root->children[11];

    string firstName = firstDcl->children[1]->tokens[1];

    string secondName = secondDcl->children[1]->tokens[1];

    cout << "wain:" << endl;

    // x8 = 8
    cout << "ldr x8, 8" << endl;
    cout << "b 12" << endl;
    cout << ".8byte 8" << endl;

    // x20 = 0
    cout << "sub x20, x20, x20" << endl;

    // Push parameter a
    cout << "stur x0, [sp, -8]" << endl;
    cout << "sub sp, sp, x8" << endl;

    // Push parameter b
    cout << "stur x1, [sp, -8]" << endl;
    cout << "sub sp, sp, x8" << endl;

    // Set the fixed frame pointer
    cout << "add x29, sp, xzr" << endl;
    
    string firstType = firstDcl->children[1]->type;
    string secondType = secondDcl->children[1]->type;

    pushX30();

    if (firstType == "long*") {
        // Second parameter contains array length
        cout << "ldur x1, [x29, 0]" << endl;
    }
    else {
        // not given an input array
        cout << "sub x1, x1, x1" << endl;
    }

    loadAndSkip("x2", "init");
    cout << "blr x2" << endl;

    popX30();

    symbolTable.table[firstName] = {firstType, 8};
    symbolTable.table[secondName] = {secondType, 0};

    int nextOffset = -8;

    generateDcls(
        dcls,
        symbolTable,
        nextOffset
    );

    generateStatements(
        statements,
        symbolTable
    );

    generateExpr(
        returnExpr,
        symbolTable
    );

    // Pop every declared variable
    for (int i = 0; i < (int)symbolTable.table.size(); ++i) {
        cout << "add sp, sp, x8" << endl;
    }

    cout << "br x30" << endl;
}


int analyzeParams(Node* node, SymbolTable& symbolTable) {

    /*
    params →
    params → paramlist
    paramlist → dcl
    paramlist → dcl COMMA paramlist
    */

    //Params -> empty
    if (node->rule == "params .EMPTY") {
        return 0;
    }

    // Params -> Paramlist;
    if (node->rule == "params paramlist") {
        return analyzeParams(node->children[0], symbolTable); // Recus into the list of params
    }

    Node* dclNode = node->children[0];
    Node* idNode = dclNode->children[1];

    string name = idNode->tokens[1];
    string type = idNode->type;

    if (node->rule == "paramlist dcl") {

        symbolTable.table[name] = {type, 16}; //This is the offset from x29 and x30 in the stack
        return 1;

    }

    //Recursive leap of faith for dcl COMMA paramlist 
    int remaining = analyzeParams(node->children[2], symbolTable); 

    symbolTable.table[name] = {type, 16 + 8 * remaining}; // x29 + x30 + all dcls after the curr one

    return 1 + remaining;
}


// LONG ID LPAREN params RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE 
void generateProcedure(Node* node, SymbolTable& symbolTable) {

    string label = symbolTable.prod;

    Node* params = node->children[3];
    Node* dcls = node->children[6];
    Node* statements = node->children[7];

    Node* returnExpr = node->children[9];


    // Get the number of params
    analyzeParams(params, symbolTable); 

    cout << "F" << label << ":" << endl; // PRint out the label

    // save reg x29, x30
    cout << "stur x29, [sp, -8]" << endl;
    cout << "sub sp, sp, x8" << endl;

    cout << "stur x30, [sp, -8]" << endl;
    cout << "sub sp, sp, x8" << endl;

    //set new fp 
    cout << "add x29, sp, xzr" << endl;

    int nextOffset = -8;

    generateDcls(dcls, symbolTable, nextOffset);
    generateStatements(statements, symbolTable);
    generateExpr(returnExpr, symbolTable);

    // Discard local variables
    cout << "add sp, x29, xzr" << endl;

    // Restore the incoming x30
    cout << "ldur x30, [sp, 0]" << endl;
    cout << "add sp, sp, x8" << endl;

    // Restore the caller's x29
    cout << "ldur x29, [sp, 0]" << endl;
    cout << "add sp, sp, x8" << endl;

    // Return with result still in x0
    cout << "br x30" << endl;

}


void generateProcedures(Node* tree, ProdTables& prodtables) {

    if (tree->rule == "procedures procedure procedures") {
        
        //Gen the prodcedure
        string procedureName = tree->children[0]->children[1]->tokens[1];
        SymbolTable& symbolTable = prodtables.prods[procedureName];

        symbolTable.prod = procedureName; // So I can pull the name for the curr prod for label 

        generateProcedure(tree->children[0], symbolTable);

        //Recurse and gen the next ProdcedureS
        generateProcedures(tree->children[1], prodtables);

    } else {

        SymbolTable& symbolTable = prodtables.prods["wain"];

        generateWain(tree->children[0], symbolTable);
    }

}


int main() {
    Node* tree = buildTree();

    if (tree == nullptr) {
        return 1;
    }

    ProdTables prodTables;
    Node* proceduresNode = nullptr;

    cout << ".import print" << endl;
    cout << ".import init" << endl;
    cout << ".import new" << endl;
    cout << ".import delete" << endl;
    cout << "b wain" << endl;

    if (tree->rule == "start BOF procedures EOF") {
        proceduresNode = tree->children.at(1);
    }
    else if (
        tree->rule == "procedures main" ||
        tree->rule == "procedures procedure procedures"
    ) {
        // tree is already the procedures node
        proceduresNode = tree;
    }
    else {
        cerr << "Unexpected root rule: "
            << tree->rule << endl;

        deleteTree(tree);
        return 1;
    }

    generateProcedures(proceduresNode, prodTables);

    deleteTree(tree);
}
