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

## Review details

The supplied MP4s show simulation progressing at fast speed while management UI was open. The fix gates simulation before a due tick, including the frame that opens a window; the existing scheduling gate also pauses animation and resets deadlines on resume. No global speed value is overwritten.

The old MPS content was 272 pixels inside a 204-pixel page. The revised page reserves 240 pixels consistently, so 13 text rows fit without covering the overlay/navigation strips. Switch children are clipped for drawing and mouse hit testing; this also fixes click interception outside the active page.

Window-light crops remain the existing art with original coordinate scale. Variant resources fall back to the corresponding base resource lighting. Street lamps are procedural and culled to the viewport and roads near constructed lots; they do not multiply image storage.

Tax adjustments use completed-year operating statistics and current cash/reserve; review cooldown is a backward-compatible optional XML field. The older service test expectation was explicitly changed because tax delegation is now independent of building delegation. New finance cases exercise that separation and monthly repeat/save limits.

The audit supplement includes the precise patch, logs and actual screenshots. Tests on a native dummy/software SDL renderer validate interaction/layout, not a Windows desktop certification. The prior Wayland backend is retained; no new physical Fedora/Windows GPU test has been performed.
