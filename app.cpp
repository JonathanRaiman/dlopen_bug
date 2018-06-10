#include "test_project/cool.h"
#include <iostream>
#include <memory>
#include <dlfcn.h>      // dynamic library loading, dlopen() etc
#include <thread>


struct ModulePointer {
    void* module_;
    std::string libname_;
    ModulePointer(const std::string& libname, bool retry);
    ~ModulePointer();
};


struct Module {
    std::shared_ptr<ModulePointer> module_ptr_;

    Module();
    Module(const std::string& libname, bool retry);
    void* module();

    template<typename T>
    T get_symbol(const std::string& name) {
        void* symbol = dlsym(module(), name.c_str());
        const char* dlsym_error = dlerror();
        if (dlsym_error != NULL) {
            std::cerr << "error loading symbol:\n" << dlsym_error << std::endl;
            exit(EXIT_FAILURE);
        }
        return reinterpret_cast<T>(symbol);
    }
};


ModulePointer::ModulePointer(const std::string& libname, bool retry) : module_(NULL), libname_(libname) {
    module_ = dlopen(libname_.c_str(), RTLD_LAZY);
    if (!module_) {
        if (!retry) {
            std::cerr << "error loading library:\n" << dlerror() << std::endl;
            exit(EXIT_FAILURE);
        } else {
            while (!module_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                module_ = dlopen(libname_.c_str(), RTLD_LAZY);
            }
        }
    }
}

ModulePointer::~ModulePointer() {
    if (module_) {
        dlclose(module_);
    }
}

Module::Module() : module_ptr_(NULL) {}
Module::Module(const std::string& libname, bool retry) :
        module_ptr_(std::make_shared<ModulePointer>(libname, retry)) {
}

void* Module::module() {
    return module_ptr_->module_;
}


int main(int argc, char* argv[]) {
	int i = 0;

	auto lib = Module("external_lib.so", false);
	auto symbol = lib.get_symbol<void*>("maker");

	auto symbol_func = reinterpret_cast<int(*)(int, int)>(symbol);

	while (true) {
		auto left = do_cool_stuff(1000);
		auto right = do_cool_stuff(1000);
		auto res = dot(left, right) + symbol_func(2, 3);
		std::cout << "i = " << i << ", res = " << res << std::endl;
		i += 1;
		if (i > 1000) {
			i = 0;
		}
	}
}
