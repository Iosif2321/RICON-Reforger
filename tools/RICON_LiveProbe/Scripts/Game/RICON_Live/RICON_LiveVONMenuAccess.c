//! TOOL_ONLY (A1-S1, codex #4459, #4463, #4465): доступ к собственному config-контроллеру радиального VON-меню.
//! m_RadialController — защищённый атрибут SCR_VONMenu без публичного getter-а. Этот modded class только возвращает
//! ссылку на существующий объект: ни нового контроллера, ни своих inputs, ни изменения поведения меню. Аддон
//! подключается лишь на время опыта через -addonsDir и не публикуется.
modded class SCR_VONMenu
{
	SCR_RadialMenuController RICON_LiveRadialController()
	{
		return m_RadialController;
	}
}
