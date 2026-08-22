/*	Renegade tt.dll
	Shader developer mode console commands
	Copyright 2009 Mark Sararu
	This file is part of the renegade tt.dll.
	CONFIDENTIAL: DO NOT USE OR DISTRIBUTE WITHOUT PERMISSION
*/

#ifndef TT_INCLUDE_SHADERDEV_CONSOLE_H
#define TT_INCLUDE_SHADERDEV_CONSOLE_H

class ResetShaders_ConsoleCommand: public ConsoleFunctionClass 
{
public:
	char *Get_Name(void)	{ return "resetshaders"; }
	char *Get_Alias(void)	{ return "rss"; }
	char *Get_Help(void)	{ return "resetshaders - <shaderdev> Reloads active shaders, may crash client"; }
	void Activate(char *);
};

class ResetDevice_ConsoleCommand: public ConsoleFunctionClass
{
public:
	char *Get_Name(void)	{ return "resetdevice"; }
	char *Get_Alias(void)	{ return "rsd"; }
	char *Get_Help(void)	{ return "resetdevice - <shaderdev> Forces device reset, causes texture reload"; }
	void Activate(char *);
};

class ResetWater_ConsoleCommand: public ConsoleFunctionClass
{
public:
	char *Get_Name(void)	{ return "resetwater"; }
	char *Get_Alias(void)	{ return "rsw"; }
	char *Get_Help(void)	{ return "resetwater - <shaderdev> Forces water system reset"; }
	void Activate(char *);
};

#endif
