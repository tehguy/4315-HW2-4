#ifndef MYPYTHON_PYFUNCTION_HPP
#define MYPYTHON_PYFUNCTION_HPP

#include <memory>
#include <utility>
#include <vector>
#include <unordered_map>

#include "pyobject.hpp"
#include "statements/pystatement.hpp"

class PyFunction {
public:
    PyFunction() = default;

    std::string getName() const { return funcName; }

    virtual void evaluate(std::vector<std::string> args);
protected:
    std::string funcName;
    std::vector<std::string> funcArgs;
    std::vector<std::string> funcSigVars;
    std::vector<std::unique_ptr<PyStatement>> funcStatements;

    //std::unordered_map<std::string, std::shared_ptr<PyObject>> funcArgs;
    std::unordered_map<std::string, std::shared_ptr<PyObject>> localVars;

    friend class PyFunctionBuilder;
    friend class FunctionModule;
};
#endif //MYPYTHON_PYFUNCTION_HPP
