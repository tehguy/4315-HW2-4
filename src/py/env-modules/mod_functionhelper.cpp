#include "mod_functionhelper.hpp"

#include "../pyenvironment.hpp"
#include "../stdfunction/stdinclude.hpp"

#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>

void FunctionModule::evaluate(const std::string &task, std::vector<boost::any> args) {
    if (task == "gen") {
        generateFunction(args);
        return;
    }
    else if (task == "setVar") {
        auto function = PyEnvironment::Instance().getFunctionStackTop();
        setFunctionVar(function, args);
        return;
    }
    else if (task == "getVar") {
        std::string funcName = PyEnvironment::Instance().getFunctionStackTop()->getName();
        std::string varName = boost::any_cast<std::string>(args[0]);

        getFunctionVar(funcName, varName);
        return;
    }
    else if (task == "evalFunc") {
        std::string sig = boost::any_cast<std::string>(args[0]);
        boost::trim(sig);

        runFunction(sig);
        return;
    }
}

void FunctionModule::evaluate(const std::string &func, const std::string &varName, PyObject &object) {
    if (func == "setVar") {
        auto function = PyEnvironment::Instance().getFunctionStackTop();
        setFunctionVar(function, varName, object);
        return;
    }
}

bool FunctionModule::varNameUsed(std::string varName) {
    auto currentFunc = PyEnvironment::Instance().getFunctionStackTop();

    auto iter = currentFunc->localVars.find(varName);

    return !(iter == currentFunc->localVars.end());
}

void FunctionModule::generateFunction(std::vector<boost::any> funcStrings) {
    std::shared_ptr<PyFunction> function = pyFunctionBuilder.init()
            .parseFunctionSignature(boost::any_cast<std::string>(funcStrings[0]))
            .parseStatements(boost::any_cast<std::string>(funcStrings[1]))
            .build();

    pyFunctions.emplace(function->getName(), function);
}

void FunctionModule::getFunctionVar(const std::string &funcName, const std::string &varName) {
    if (!varNameUsed(varName)) {
        PyEnvironment::Instance().pyConsole.logError(PyErrors::ERR_TYPE::NameErrorFunction, varName.c_str(), funcName.c_str());
        return;
    }

    std::shared_ptr<PyObject> pyObject = PyEnvironment::Instance().getFunctionStackTop()->localVars.at(varName);
    PyEnvironment::Instance().funcReturnStack.push(pyObject);
}

void FunctionModule::setFunctionVar(std::shared_ptr<PyFunction> function, std::vector<boost::any> args) {
    auto vartype = boost::any_cast<PyConstants::VarTypes>(args[0]);
    const std::string varName = boost::any_cast<std::string>(args[1]);
    boost::any value = args[2];

    bool nameUsed = varNameUsed(varName);

    if (nameUsed && vartype != PyConstants::VarTypes::NONE) {
        PyEnvironment::Instance().mutatedVars.push_back(varName);

        if (function->localVars.at(varName)->type == vartype) {
            modifyFunctionVar(function, vartype, varName, value);
        } else {
            mutateFunctionVar(function, vartype, varName, value);
        }
    } else if (!nameUsed && vartype != PyConstants::VarTypes::NONE) {
        createFunctionVar(function, vartype, varName, value);
    }
}

void FunctionModule::setFunctionVar(std::shared_ptr<PyFunction> function, const std::string &varName, PyObject &object) {
    bool nameUsed = varNameUsed(varName);

    if (nameUsed && object.type != PyConstants::VarTypes::NONE) {
        PyEnvironment::Instance().mutatedVars.push_back(varName);

        if (function->localVars.at(varName)->type == object.type) {
            modifyFunctionVar(function, object.type, varName, object.getData<boost::any>());
        } else {
            mutateFunctionVar(function, object.type, varName, object.getData<boost::any>());
        }
    } else if (!nameUsed && object.type != PyConstants::VarTypes::NONE) {
        createFunctionVar(function, object.type, varName, object.getData<boost::any>());
    }
}

void FunctionModule::mutateFunctionVar(std::shared_ptr<PyFunction> function, PyConstants::VarTypes vartype,
                                       const std::string &varName, boost::any value) {
    switch ( vartype ) {
        case PyConstants::VarTypes::NUMBER: {
            function->localVars.at(varName) = std::make_shared<PyInt>(boost::any_cast<int>(value));
            break;
        }
        case PyConstants::VarTypes::BOOL: {
            function->localVars.at(varName) = std::make_shared<PyBool>(boost::any_cast<bool>(value));
            break;
        }
        case PyConstants::VarTypes::STRING: {
            function->localVars.at(varName) = std::make_shared<PyString>(boost::any_cast<std::string>(value));
            break;
        }
        default:
            break;
    }
}

void FunctionModule::modifyFunctionVar(std::shared_ptr<PyFunction> function, PyConstants::VarTypes vartype,
                                       const std::string &varName, boost::any value) {
    switch ( vartype ) {
        case PyConstants::VarTypes::NUMBER: {
            function->localVars.at(varName)->setData<int>(boost::any_cast<int>(value));
            break;
        }
        case PyConstants::VarTypes::BOOL: {
            function->localVars.at(varName)->setData<bool>(boost::any_cast<bool>(value));
            break;
        }
        case PyConstants::VarTypes::STRING: {
            function->localVars.at(varName)->setData<std::string>(boost::any_cast<std::string>(value));
            break;
        }
        default:
            break;
    }
}

void FunctionModule::createFunctionVar(std::shared_ptr<PyFunction> function, PyConstants::VarTypes vartype,
                                       const std::string &varName, boost::any value) {
    switch ( vartype ) {
        case PyConstants::VarTypes::NUMBER: {
            function->localVars.insert(
                    std::make_pair(
                            varName,
                            std::make_shared<PyInt>(boost::any_cast<int>(value))
                    ));
            break;
        }
        case PyConstants::VarTypes::BOOL: {
            function->localVars.insert(
                    std::make_pair(
                            varName,
                            std::make_shared<PyBool>(boost::any_cast<bool>(value))
                    ));
            break;
        }
        case PyConstants::VarTypes::STRING: {
            function->localVars.insert(
                    std::make_pair(
                            varName,
                            std::make_shared<PyString>(boost::any_cast<std::string>(value))
                    ));
            break;
        }
        default:
            break;
    }
}

void FunctionModule::runFunction(const std::string &sig) {
    boost::regex regex{R"(^[A-Za-z_]+([\w]+)?([ ]+)?)"};
    boost::smatch match;
    std::vector<std::string> results;

    std::string sigcop = sig;
    std::string funcName;

    if (boost::regex_search(sigcop, match, regex)) {
        funcName = match.str();
        boost::trim(funcName);

        std::string::size_type i = sigcop.find(funcName);
        if (i != std::string::npos) {
            sigcop.erase(i,funcName.size());
        }
    }

    regex = R"(("[\w ,\.\-=+*\/!@#$%^&*()|{}\[\];\'<>?]+")|([A-Za-z_]+[A-Za-z0-9_]*)|([\d]+(\.[\d]+)?))";

    for (boost::sregex_iterator i = boost::sregex_iterator(sig.begin(), sig.end(), regex);
         i != boost::sregex_iterator(); ++i) {
        boost::smatch m = *i;

        results.push_back(m.str());
    }

    PyEnvironment::Instance().isRecursive = PyEnvironment::Instance().getFunctionStackTop()->funcName == funcName;

    auto function = pyFunctions.at(funcName);
    PyEnvironment::Instance().pushOntoFunctionStack(function);
    PyEnvironment::Instance().getFunctionStackTop()->eval(results);
    PyEnvironment::Instance().popFunctionStack();
}

void FunctionModule::initStandardFunctions() {
    std::shared_ptr<PyFunction> print = std::make_shared<StdPrint>();
    pyFunctions.insert({"print", print});
}
