package artery.application;

simple ItsRadioModule extends RadioModule
{
	parameters:
		@class(artery::ItsRadioModule);

		int vanetzaDccQueueLength = default(2);
}
