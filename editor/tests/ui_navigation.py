# Shared navigation for the Studio UI tests. Since RC2 the editor groups its workspaces into
# five sections (Maps, Artwork, Story, Audio, Game) with contextual sub-tabs, so a tool's tab
# only exists once its section is open (see editor/ui/navigation.js).


def open_tool(page, section, tool):
    """Open `section` in the primary navigation, then its `tool` sub-tab.

    Sections with a single tool (Audio) hide their sub-tabs, so only the section is clicked.
    Waits until the requested tool is the active view.
    """
    page.locator(f"#primaryNavigation [data-workspace={section}]").click()
    tabs = page.locator("#workspaceSections")
    if tabs.is_visible():
        tabs.locator(f"[data-section={tool}]").click()
    page.wait_for_function(
        """([section, tool]) => {
            const group = workspaceGroups.find((g) => g.id === section);
            return document.body.dataset.workspace === section &&
                group.views.some(([id]) => id === tool) && view === tool;
        }""",
        arg=[section, tool],
    )
