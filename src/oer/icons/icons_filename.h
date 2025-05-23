/*
   icons/icons_filename.h
   https://github.com/yinmus/ifm.git
    
*/

#ifndef ICONS_FILENAME_H
#define ICONS_FILENAME_H

typedef struct {
  const char *filename;
  const char *icon;
  const char *color;
} FilenameMapping;

static const FilenameMapping filename_mappings[] = {

    {".Xauthority", "\uf369", "#E54D18"},                   {".Xresources", "\uf369", "#E54D18"},                   {".xinitrc", "\uf369", "#E54D18"},
    {".babelrc", "\ue639", "#CBCB41"},                      {".bash_profile", "\ue615", "#89E051"},                 {".bashrc", "\ue615", "#89E051"},
    {".clang-format", "\ue615", "#6D8086"},                 {".clang-tidy", "\ue615", "#6D8086"},                   {".codespellrc", "\U000f04c6", "#35DA60"},
    {".condarc", "\ue715", "#43B02A"},                      {".dockerignore", "\U000f0868", "#458EE6"},             {".ds_store", "\ue615", "#41535B"},
    {".editorconfig", "\ue652", "#FFF2F2"},                 {".env", "\uf462", "#FAF743"},                          {".eslintignore", "\ue655", "#4B32C3"},
    {".eslintrc", "\ue655", "#4B32C3"},                     {".git-blame-ignore-revs", "\ue702", "#F54D27"},        {".gitattributes", "\ue702", "#F54D27"},
    {".gitconfig", "\ue615", "#F54D27"},                    {".gitignore", "\ue702", "#F54D27"},                    {".gitlab-ci.yml", "\uf296", "#E24329"},
    {".gitmodules", "\ue702", "#F54D27"},                   {".gtkrc-2.0", "\uf362", "#FFFFFF"},                    {".gvimrc", "\ue62b", "#019833"},
    {".justfile", "\uf0ad", "#6D8086"},                     {".luacheckrc", "\ue615", "#00A2FF"},                   {".luaurc", "\ue615", "#00A2FF"},
    {".mailmap", "\U000f02a2", "#F54D27"},                  {".nanorc", "\ue838", "#440077"},                       {".npmignore", "\ue71e", "#E8274B"},
    {".npmrc", "\ue71e", "#E8274B"},                        {".nuxtrc", "\U000f1106", "#00C58E"},                   {".nvmrc", "\ue718", "#5FA04E"},
    {".pre-commit-config.yaml", "\U000f06e2", "#F8B424"},   {".prettierignore", "\ue6b4", "#4285F4"},               {".prettierrc", "\ue6b4", "#4285F4"},
    {".prettierrc.cjs", "\ue6b4", "#4285F4"},               {".prettierrc.js", "\ue6b4", "#4285F4"},                {".prettierrc.json", "\ue6b4", "#4285F4"},
    {".prettierrc.json5", "\ue6b4", "#4285F4"},             {".prettierrc.mjs", "\ue6b4", "#4285F4"},               {".prettierrc.toml", "\ue6b4", "#4285F4"},
    {".prettierrc.yaml", "\ue6b4", "#4285F4"},              {".prettierrc.yml", "\ue6b4", "#4285F4"},               {".pylintrc", "\ue615", "#6D8086"},
    {".settings.json", "\ue70c", "#854CC7"},                {".vimrc", "\ue62b", "#019833"},                        {".viminfo", "\ue62b", "#019833"},
    {"xinitrc", "\uf369", "#E54D18"},                       {".xsession", "\uf369", "#E54D18"},                     {".zprofile", "\ue615", "#89E051"},
    {".zshenv", "\ue615", "#89E051"},                       {".zshrc", "\ue615", "#89E051"},                        {"AUTHORS", "\uedca", "#A172FF"},
    {"AUTHORS.txt", "\uedca", "#A172FF"},                   {"Directory.Build.props", "\ue615", "#00A2FF"},         {"Directory.Build.targets", "\ue615", "#00A2FF"},
    {"Directory.Packages.props", "\ue615", "#00A2FF"},      {"FreeCAD.conf", "\uf336", "#CB333B"},                  {"Gemfile", "\ue791", "#701516"},
    {"PKGBUILD", "\uf303", "#0F94D2"},                      {"PrusaSlicer.ini", "\uf351", "#EC6B23"},               {"PrusaSlicerGcodeViewer.ini", "\uf351", "#EC6B23"},
    {"QtProject.conf", "\uf375", "#40CD52"},                {"_gvimrc", "\ue62b", "#019833"},                       {"_vimrc", "\ue62b", "#019833"},
    {"brewfile", "\ue791", "#701516"},                      {"bspwmrc", "\uf355", "#2F2F2F"},                       {"build", "\ue63a", "#89E051"},
    {"build.gradle", "\ue660", "#005F87"},                  {"build.zig.zon", "\ue6a9", "#F69A1B"},                 {"bun.lock", "\ue76f", "#EADCD1"},
    {"bun.lockb", "\ue76f", "#EADCD1"},                     {"cantorrc", "\uf373", "#1C99F3"},                      {"checkhealth", "\U000f04d9", "#75B4FB"},
    {"cmakelists.txt", "\ue794", "#DCE3EB"},                {"code_of_conduct", "\uf4ae", "#E41662"},               {"code_of_conduct.md", "\uf4ae", "#E41662"},
    {"commit_editmsg", "\ue702", "#F54D27"},                {"commitlint.config.js", "\U000f0718", "#2B9689"},      {"commitlint.config.ts", "\U000f0718", "#2B9689"},
    {"compose.yaml", "\U000f0868", "#458EE6"},              {"compose.yml", "\U000f0868", "#458EE6"},               {"config", "\ue615", "#6D8086"},
    {"containerfile", "\U000f0868", "#458EE6"},             {"copying", "\ue60a", "#CBCB41"},                       {"copying.lesser", "\ue60a", "#CBCB41"},
    {"docker-compose.yaml", "\U000f0868", "#458EE6"},       {"docker-compose.yml", "\U000f0868", "#458EE6"},        {"dockerfile", "\U000f0868", "#458EE6"},
    {"eslint.config.cjs", "\ue655", "#4B32C3"},             {"eslint.config.js", "\ue655", "#4B32C3"},              {"eslint.config.mjs", "\ue655", "#4B32C3"},
    {"eslint.config.ts", "\ue655", "#4B32C3"},              {"ext_typoscript_setup.txt", "\ue772", "#FF8700"},      {"favicon.ico", "\ue623", "#CBCB41"},
    {"fp-info-cache", "\uf34c", "#FFFFFF"},                 {"fp-lib-table", "\uf34c", "#FFFFFF"},                  {"gnumakefile", "\ue779", "#6D8086"},
    {"go.mod", "\ue627", "#519ABA"},                        {"go.sum", "\ue627", "#519ABA"},                        {"go.work", "\ue627", "#519ABA"},
    {"gradle-wrapper.properties", "\ue660", "#005F87"},     {"gradle.properties", "\ue660", "#005F87"},             {"gradlew", "\ue660", "#005F87"},
    {"groovy", "\ue775", "#4A687C"},                        {"gruntfile.babel.js", "\ue611", "#E37933"},            {"gruntfile.coffee", "\ue611", "#E37933"},
    {"gruntfile.js", "\ue611", "#E37933"},                  {"gruntfile.ts", "\ue611", "#E37933"},                  {"gtkrc", "\uf362", "#FFFFFF"},
    {"gulpfile.babel.js", "\ue610", "#CC3E44"},             {"gulpfile.coffee", "\ue610", "#CC3E44"},               {"gulpfile.js", "\ue610", "#CC3E44"},
    {"gulpfile.ts", "\ue610", "#CC3E44"},                   {"hypridle.conf", "\uf359", "#00AAAE"},                 {"hyprland.conf", "\uf359", "#00AAAE"},
    {"hyprlock.conf", "\uf359", "#00AAAE"},                 {"hyprpaper.conf", "\uf359", "#00AAAE"},                {"i18n.config.js", "\U000f05ca", "#7986CB"},
    {"i18n.config.ts", "\U000f05ca", "#7986CB"},            {"i3blocks.conf", "\uf35a", "#E8EBEE"},                 {"i3status.conf", "\uf35a", "#E8EBEE"},
    {"index.theme", "\uee72", "#2DB96F"},                   {"ionic.config.json", "\ue7a9", "#4F8FF7"},             {"justfile", "\uf0ad", "#6D8086"},
    {"kalgebrarc", "\uf373", "#1C99F3"},                    {"kdeglobals", "\uf373", "#1C99F3"},                    {"kdenlive-layoutsrc", "\uf33c", "#83B8F2"},
    {"kdenliverc", "\uf33c", "#83B8F2"},                    {"kritadisplayrc", "\uf33d", "#F245FB"},                {"kritarc", "\uf33d", "#F245FB"},
    {"license", "\ue60a", "#D0BF41"},                       {"license.md", "\ue60a", "#D0BF41"},                    {"license.txt", "\ue60a", "#D0BF41"},
    {"lxde-rc.xml", "\uf363", "#909090"},                   {"lxqt.conf", "\uf364", "#0192D3"},                     {"makefile", "\ue779", "#6D8086"},
    {"mix.lock", "\ue62d", "#A074C4"},                      {"mpv.conf", "\uf36e", "#3B1342"},                      {"node_modules", "\ue718", "#E8274B"},
    {"nuxt.config.cjs", "\U000f1106", "#00C58E"},           {"nuxt.config.js", "\U000f1106", "#00C58E"},            {"nuxt.config.mjs", "\U000f1106", "#00C58E"},
    {"nuxt.config.ts", "\U000f1106", "#00C58E"},            {"package-lock.json", "\ue71e", "#7A0D21"},             {"package.json", "\ue71e", "#E8274B"},
    {"platformio.ini", "\ue682", "#F6822B"},                {"pom.xml", "\ue674", "#7A0D21"},                       {"prettier.config.cjs", "\ue6b4", "#4285F4"},
    {"prettier.config.js", "\ue6b4", "#4285F4"},            {"prettier.config.mjs", "\ue6b4", "#4285F4"},           {"prettier.config.ts", "\ue6b4", "#4285F4"},
    {"procfile", "\ue607", "#A074C4"},                      {"py.typed", "\ue606", "#FFBC03"},                      {"rakefile", "\ue791", "#701516"},
    {"readme", "\uf48a", "#EDEDED"},                        {"readme.md", "\uf48a", "#EDEDED"},                     {"rmd", "\ue609", "#519ABA"},
    {"robots.txt", "\U000f06a9", "#5D7096"},                {"security", "\U000f0483", "#BEC4C9"},                  {"security.md", "\U000f0483", "#BEC4C9"},
    {"settings.gradle", "\ue660", "#005F87"},               {"svelte.config.js", "\ue697", "#FF3E00"},              {"sxhkdrc", "\uf355", "#2F2F2F"},
    {"sym-lib-table", "\uf34c", "#FFFFFF"},                 {"tailwind.config.js", "\U000f13ff", "#20C2E3"},        {"tailwind.config.mjs", "\U000f13ff", "#20C2E3"},
    {"tailwind.config.ts", "\U000f13ff", "#20C2E3"},        {"tmux.conf", "\uebc8", "#14BA19"},                     {"tmux.conf.local", "\uebc8", "#14BA19"},
    {"tsconfig.json", "\ue69d", "#519ABA"},                 {"unlicense", "\ue60a", "#D0BF41"},                     {"vagrantfile", "\uf2b8", "#1563FF"},
    {"vercel.json", "\ue8d3", "#FFFFFF"},                   {"vlcrc", "\U000f057c", "#EE7A00"},                     {"webpack", "\U000f072b", "#519ABA"},
    {"weston.ini", "\uf367", "#FFBB01"},                    {"workspace", "\ue63a", "#89E051"},                     {"wrangler.jsonc", "\ue792", "#F48120"},
    {"wrangler.toml", "\ue792", "#F48120"},                 {"xmobarrc", "\uf35e", "#FD4D5D"},                      {"xmobarrc.hs", "\uf35e", "#FD4D5D"},
    {"xmonad.hs", "\uf35e", "#FD4D5D"},                     {"xorg.conf", "\uf369", "#E54D18"},                     {"xsettingsd.conf", "\uf369", "#E54D18"}

};

#endif
