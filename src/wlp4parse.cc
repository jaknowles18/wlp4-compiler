#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <memory>
#include "wlp4data.h"

using namespace std;


istringstream readInWLP() {

    istringstream cfg(WLP4_COMBINED);

    return cfg;
}

struct Tree {
    
    string lhs;
    deque<shared_ptr<Tree>> rhs;

};


struct Rule {
    string lhs;
    vector<string> rhs;
    string fullRule;   // Full production line, e.g. "expr term"

};

vector<string> split(const string &line) {
    vector<string> tokens;
    stringstream ss(line);
    string token;

    while (ss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

void printProgress(const vector<string> &reduction, const deque<string> &input) {
    bool first = true;

    for (const string &s : reduction) {
        if (!first) cout << " ";
        cout << s;
        first = false;
    }

    if (!first) cout << " ";
    cout << ".";
    first = false;

    for (const string &s : input) {
        cout << " " << s;
    }

    cout << endl;
}

void printTree(const Tree &tree) {
    cout << tree.lhs << endl;

    for (const auto &child : tree.rhs) {
        printTree(*child);
    }
}

int main() {
    string line = "";
    vector<Rule> rules;
    vector<string> reduction;
    vector<int> states;
    map<pair<int, string>, int> transitions;
    map<pair<int, string>, int> reductions;
    vector<shared_ptr<Tree>> treeStack; // Store the trees

    istringstream wlp4 = readInWLP();

    getline(wlp4, line); // Read .CFG 

    //Rule storing itteration
    while(getline(wlp4, line)) {

        //Stop storing rules if .INPUT starts
        if(line == ".TRANSITIONS") {
            break;
        }

        //Set up new rule strutc 
        Rule curr_rule; 
        curr_rule.fullRule = line; // important for printing the parse tree

        vector<string> tokens = split(line); // split the tokens so it is easier to push
        curr_rule.lhs = tokens[0]; // Set the lhs of the component

        //Set up the rhs of the rule
        for(int i = 1 ; i < (int)tokens.size() ; ++i) {
            
            if (tokens[i] != ".EMPTY") {
                curr_rule.rhs.push_back(tokens[i]); 
            }
        }
        rules.push_back(curr_rule); // Store that rule 

    }


/* RULE DEBUGGING 
    for (auto rule : rules) {
        cout << rule.lhs << "  ";
        for (auto rhs : rule.rhs) {
            cout << " " << rhs;
        }
        cout << endl;
    }
        */

/*
    //Read .INPUT
    deque<string> input;

    while (getline(wlp4, line)) {
        vector<string> input_tokens = split(line); //Store input as tokens to push easy

        if (line == ".TRANSITIONS") { break; }

        for (auto &s : input_tokens) {
            input.push_back(s);
        }

    }
        */

    //Store Transitions:
    while(getline(wlp4, line)) {

        if (line == ".REDUCTIONS") {
            break;
        }

        vector<string> curr_tran = split(line);

        if ((int)curr_tran.size() != 3) {
            cerr << "ERROR: malformed parser transition table" << endl;
            return 1;
        }

        int fromState = stoi(curr_tran[0]);
        string symbol = curr_tran[1];
        int toState = stoi(curr_tran[2]);

        transitions[{fromState, symbol}] = toState;

    }

    //Store Reductions
    int acceptState = -1;
    int acceptRule = -1;

// Store Reductions
    while (getline(wlp4, line)) {
        if (line == ".END") {
            break;
        }   

        vector<string> curr_red = split(line);

        if ((int)curr_red.size() != 3) {
            cerr << "ERROR: malformed parser reduction table" << endl;
            return 1;
        }

        int stateNum = stoi(curr_red[0]);
        int ruleNum = stoi(curr_red[1]);
        string lookahead = curr_red[2];

        if (lookahead == ".ACCEPT") {
            acceptState = stateNum;
            acceptRule = ruleNum;
        } else {
          reductions[{stateNum, lookahead}] = ruleNum;
        }   
    }


    /*
    while(true) {

        if((int)input.size() == 0) {
            break;
        }
    }
*/
/*
// DEBUG
cout << "TRANSITIONS MAP:" << endl;

for (const auto &entry : transitions) {
    int state = entry.first.first;
    string symbol = entry.first.second;
    int nextState = entry.second;

    cout << "(" << state << ", " << symbol << ") -> " << nextState << endl;
}

cout << endl;

cout << "REDUCTIONS MAP:" << endl;

for (const auto &entry : reductions) {
    int state = entry.first.first;
    string lookahead = entry.first.second;
    int ruleNumber = entry.second;

    cout << "(" << state << ", " << lookahead << ") -> rule " << ruleNumber << endl;
}
    */

    //Create the output 
    states.push_back(0);
    int shifted = 0;
    deque<pair<string, string>> input;

    input.push_back({"BOF", "BOF"});

    while (getline(cin, line)) {
        vector<string> input_tokens = split(line);

        if (!input_tokens.empty()) {
            if (input_tokens.size() != 2) {
                cerr << "ERROR: malformed token input" << endl;
                return 1;
            }
            string kind = input_tokens[0];
            string lexeme = input_tokens[1];

            input.push_back({kind, lexeme});
        }
    }

     input.push_back({"EOF", "EOF"});

    while(true) {

        int current_state = states.back();

        //Check accept before setting lookahead
        if(current_state == acceptState) {
            const Rule &rule = rules[acceptRule];
            auto curr_reduction = make_shared<Tree>();
            curr_reduction->lhs = rule.fullRule;

            for (int i = 0; i < (int)rule.rhs.size(); ++i) {
                reduction.pop_back();
                states.pop_back();

                auto child = treeStack.back();
                treeStack.pop_back();

                curr_reduction->rhs.push_front(child);
            }

            treeStack.push_back(curr_reduction);
            break;
        }

        string look_ahead = input.front().first; // Starts as BOF
        pair<int, string> key = {current_state, look_ahead};

        // reduce case
        if (reductions.count(key)) {

            // Store what rule currently reducing for 
            int ruleNum = reductions[key];
            const Rule &rule = rules[ruleNum];

            auto curr_reduction = make_shared<Tree>();
            curr_reduction->lhs = rule.fullRule;

            // Pop RHS symbols and matching states
            for (int i = 0; i < (int)rule.rhs.size(); ++i) {
                //Pop off the stacks 
                reduction.pop_back();
                states.pop_back();

                //Pop and store the current child tree on the back of the tree stack 
                auto child = treeStack.back();
                treeStack.pop_back();

                curr_reduction->rhs.push_front(child);
                
            }

            // Push LHS symbol
            reduction.push_back(rule.lhs);
            treeStack.push_back(curr_reduction); // Re-place the child onto the tree stack 

            // Use DFA transition on the LHS
            int stateAfterPop = states.back();
            pair<int, string> gotoKey = {stateAfterPop, rule.lhs};

            int nextState = transitions[gotoKey];
            states.push_back(nextState);
            
        } else if (transitions.count(key)) { // Shift case

            pair<string, string> token = input.front();
            input.pop_front();

            string kind = token.first;
            string lexeme = token.second;

            reduction.push_back(kind);

            //Since it is just being pushed on treat it like a leaf untill it hits a reduction case
            auto leaf_node = make_shared<Tree>();
            leaf_node->lhs = kind + " " + lexeme;
            treeStack.push_back(leaf_node);

            int nextState = transitions[key];
            states.push_back(nextState);

            // BOF and EOF do not count for error position
            if (kind != "BOF" && kind != "EOF") {
                shifted++;
            }

        } else {
            cerr << "ERROR at " << shifted + 1 << endl;
            return 1;
        }

    }

    if (treeStack.empty()) {
        cerr << "ERROR: parser produced no syntax tree" << endl;
        return 1;
    }

    printTree(*treeStack.back());
    return 0;
}
