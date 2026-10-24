//! TOOL_ONLY (A1-S1, codex #4459, #4463, #4465, #4477): доступ к собственному config-контроллеру радиального VON-меню.
//! m_RadialController — защищённый config-атрибут SCR_VONMenu без публичного getter-а. Этот modded class только
//! возвращает ссылку на существующий объект: ни нового контроллера, ни своих inputs, ни изменения поведения меню.
//! SCR_VONMenu — config-класс с [BaseContainerProps()] (SCR_VONMenu.c:2); прогон A1-S1b (21:41) показал, что modded
//! class без этого атрибута ломает загрузку атрибута m_VONMenu у SCR_VONController («WORLD (E): Unknown keyword/data
//! 'm_VONMenu'», меню не создаётся), поэтому атрибут повторён. Аддон подключается лишь на время опыта и не публикуется.
[BaseContainerProps()]
modded class SCR_VONMenu
{
	SCR_RadialMenuController RICON_LiveRadialController()
	{
		return m_RadialController;
	}
}
