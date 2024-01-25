## Windows build

### Easy Building with Craft

Necessary tools:
* Windows 10 or 11
* Python 3.9
* Powershell
  * with Remote Script permissions: Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
  * 
* MSVC 2019 with the following components:
  * Desktop Development with C++
  * C++ ATL
  * Windows SDK
* Craft 

Make sure your windows 10 or 11 is in `Developer` mode:
`https://learn.microsoft.com/en-us/windows/apps/get-started/enable-your-device-for-development`

Install craft by running the following command at PowerShell:
`iex ((new-object net.webclient).DownloadString('https://raw.githubusercontent.com/KDE/craft/master/setup/install_craft.ps1'))`

Start the Craft Shell:
`C:\CraftRoot\craft\craftenv.ps1`

If you want to build from master (Currently suggested), set the craft option:
`craft —set version=master codevis`

Finall, ask Craft to build Codevis:
`craft codevis`
