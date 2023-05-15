# BinaryArt
BinaryArt unfinished but working as intended building onto the binary instruction execution now.

Priority:
- Need to create functionality to determine sys architecture for the detrmination of binary encoding for each machine instruction based on if its x86 or x64 bit arch.





- Next step after the functions are finished is to start adding thread concurrency including async and promises for certain thread tasks as 
well as including safe gaurds through atomic fences,gaurds and locks
for the prevention of race conditions in certain situations then deal with resource efficency and process hogging(i.e allocate memory into other processes to run from.

other efficeny ideas :

(suss prevention:

- Resource use Obfuscation
- signature swapping techniques
- OS/IO Resource utilisation
- Process upkeep ( managment of resources )
- perma memory reference
- perma reserve space
- dynamic address swapping on memory location av scans. 
- process hollowing 
- non enviroment base foundation / parent , child nested within kernal.
- neural botnet upkeep via client to network signal hibernated activation.
- non relational activation of child processes (absent direct parent to child process inheritance /relations to code execution paths etc). 

Note:
- once the logic is done the organisation of the code , modules and headers will start to come into play for a more efficent program , class's require alot of space so maybe keeping it procedural for now will keep the memory consumption low.

additional features for a later date :
- finding os enviroments based on macro definition inclusions as well as certain files exisitig ie. dlls
- making it so that the enviroment and debugging can be detected easily using enums for placeholder status representation.
- checking for active and total cores and other virtualisation detection techniques.
- make it so the binary instructions are self modifiable on runtime based on dynamic enviroment factors and action results .
- implement ooperating sys fuzzing system for possible zero day discovery (will take alot of time)

- assembly execution async with other assembly instructions being created.

- final goal in summary make it so this becomes self learning and fully dynamic through the use of a embedded neural network for branches of dynamic actions based on previous factors taking into account then using other branches to form new connections for more sophiscticated logic and problem solving within the binary production as well as the overall program self discovering logic.
