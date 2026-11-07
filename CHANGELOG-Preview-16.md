# City Evolution — Preview 16

Windows feedback fixes, 7 November 2026.

- City Hall, Budget, Help and other management windows pause simulation while open. Closing restores the selected simulation speed without a catch-up burst. Controls for pedestrians and plan focus never change the date directly. Window titles identify the pause.
- Repaired the right HUD overlap: information pages are 240 pixels high inside a matching 336-pixel HUD; 14-point text stays readable, and page content cannot paint or take clicks outside its bounds. Navigation/overlay buttons retain separate rows.
- Plan focus shows the selected focus. Proposed plans cycle Livable city / Skills and education / Technology hub; active plans show a locked focus and explain that their approved targets remain in effect.
- Added highlighted Auto-adjust taxes and Financial advisor controls. Automatic income tax works independently of mayor building autonomy. Enabling performs one review; further reviews occur monthly, at most one percentage point per month, under the player's ceiling. Repeated toggles cannot accelerate adjustments. Manual tax changes disable automation. Goods taxes and service funding have separate controls. Review history survives save/load.
- Financial advisor reports prior-year operating income/costs, excludes construction, flags deficits/social spending/low reserves, and recommends connecting jobs and supplying idle workplaces rather than blindly adding buildings. Advice does not secretly construct or cut services.
- Brighter ambient night rendering, stronger window lighting, reduced late-night fade, dim lantern fallback in occupied housing without electricity, and warm lamps along settled streets. No duplicate night sprite sets or new large image assets. Empty housing and unpowered electrical services can remain dark.
- Fixed a fresh Git checkout build issue: libxml++ configuration is generated in the build tree rather than depending on an ignored generated header.

Validation: nine simulation diagnostics including finance, GUI style/layout/art checks, native graphical walkthrough, advisor interaction and date-pixel pause check. Windows cross-built, native launcher resource/dependency checked. Windows physical hardware and the user's exact save have not been reproduced here. No claim of a signed Windows executable.
