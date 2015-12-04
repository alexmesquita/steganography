#include <err.h>
#include <iostream>
#include <fstream>

using namespace std;

string get_key(string path)
{
	string key = "";
	ifstream file_key(path);
	if (file_key.is_open())
	{
		getline(file_key,key);
		file_key.close();
	}
	else
	{
		errx(1, "It was not possible to open the key file");
	}
	return key;
}

int main(int argc, string argv[])
{
	string key;
	switch(argc)
	{
		case 3:
			key = get_key(argv[2]);
			break;
		default:
			errx(1, "invalid arguments");
	}

	cout << key << endl;

	return 0;
}
