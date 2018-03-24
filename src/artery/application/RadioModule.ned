package artery.application;

moduleinterface IRadioModule
{
}

simple RadioModule like IRadioModule
{
	parameters:
		@class(artery::RadioModule);

		string radioDriverModule;

		int radioChannel;

	gates:
		input radioDriverIn;
		output radioDriverOut;
}
