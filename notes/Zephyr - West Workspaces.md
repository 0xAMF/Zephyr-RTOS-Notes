## West Manifest
Manifest files are used to manage external dependencies required by zephyr.
> _West_ solves this by using a so-called [_manifest_ file](https://docs.zephyrproject.org/latest/develop/west/manifest.html), which is nothing else than a list of external dependencies - and then some. _West_ uses this manifest file to basically act like a package manager for your _C_ project, similar to what [`cargo`](https://doc.rust-lang.org/stable/cargo/) does with its `cargo.toml` files for [_Rust_](https://www.rust-lang.org/).

```
workspace/
└── app
    └── west.yml
```
The `workspace` directory is called the *"topdir"*, and the `app` directory is called the *manifest repository*.
## Initializing Workspace
We _initialize_ the _West_ workspace using [`west init`](https://docs.zephyrproject.org/latest/develop/west/basics.html#west-init-and-west-update). There are two ways to initialize a _West_ workspace:
- **Locally**, using the `-l` or `--local` flag: This assumes that your manifest repository already exists in your filesystem, e.g., you already used `git clone` to populate it in the _topdir_.
- **Remotely**, by specifying the URL to the manifest repository using the argument `-m`. With this argument, _West_ clones the manifest repository into the _topdir_ for you before initializing the workspace.
```bash
# inside workspace/app
west init --local
```
When initializing a workspace, _West_ creates a `.west` directory in the _topdir_, which in turn contains a configuration file.
```
├── app
│   └── west.yml
└── .west
    └── config
```
Content of `.west/config`
```yml
[manifest]
path = app
file = west.yml
```
The location of the `.west` folder “marks” the _topdir_ and thus the _West_ workspace root directory. Within this file, we can see that _West_ stores the location and name of the manifest file. Modifying this file - or any file within the `.west` folder - is not recommended, since some of _West’s_ commands might no longer work as expected.
The content of the `.west/config` file can be changed through `west config` command.
For example, we configure west to build for a board of our choice using the following command:
```bash
west config build.board qemu_x86_64
```
`.west/config`
```yml
[manifest]
path = app
file = west.yml

[build]
board = qemu_x86_64
```
we can now see that `west config` by default uses this _local_ configuration file to store its configuration options. _West_ also supports storing configuration options globally or even system-wide. Have a look at the [official documentation](https://docs.zephyrproject.org/latest/develop/west/config.html) in case you need to know more.
## Adding Projects
In your manifest file, you can use the `manifest.projects` key to define all Git repositories that you want to have in your workspace.
```yml
manifest:
  # minimum required version to parse this file
  version: 0.8

  projects:
    - name: doxygen-awesome
      url: https://github.com/jothepro/doxygen-awesome-css
      revision: main
      path: deps/utilities/doxygen-awesome-css
```
Every _project_ at least has a **unique** name. Typically, you’ll also specify the `url` of the repository, but there are several options, e.g., you could use the [`remotes` key](https://docs.zephyrproject.org/latest/develop/west/manifest.html#remotes) to specify a list of URLs and add specify the `remote` that is used for your project. You can find examples and a detailed explanation of all available options in the [official documentation for the `projects` key](https://docs.zephyrproject.org/latest/develop/west/manifest.html#projects).
- `revision` key: tells `west` to check out either a specific _branch_, _tag_, or commit hash.
- `path` key is also an optional key that tells _West_ the _relative_ path to the _topdir_ that it should use when cloning the project. Without specifying the `path`, _West_ uses the project’s `name` as the path. Notice that you’re **not** allowed to specify a path that is outside of the _topdir_ and thus _West_ workspace.
Now run `west update` to fetch the projects we added:
```bash
$ west update
=== updating doxygen-awesome (deps/utilities/doxygen-awesome-css):
--- doxygen-awesome: initializing
Initialized empty Git repository in /home/fyd/embedded/workspace/deps/utilities/doxygen-awesome-css/.git/
--- doxygen-awesome: fetching, need revision main
# .....
```
Now we should have `doxygen-awesome` in the path we specified:
```bash
$ tree --dirsfirst -a -L 3 --filelimit 8
.
├── app
│   └── west.yml
├── deps
│   └── utilities
│       └── doxygen-awesome-css
└── .west
    └── config
```
To have a stable workspace we shouldn't have our `revision` as main, because it gets updated regularly, instead we should specify a release or a tag.
```yml
manifest:
  # minimum required version to parse this file
  version: 0.8

  projects:
    - name: doxygen-awesome
      url: https://github.com/jothepro/doxygen-awesome-css
      revision: v2.3.4
      path: deps/utilities/doxygen-awesome-css
```
Now we run `west update` to change the version of our dependency.
## Creating a Workspace From a Repository
In Zephyr’s official documentation for _West_, the first example call to `west init` uses the `-m` argument to specify the manifest repository’s URL. In the previous section, we’ve seen how we can _create_ such a manifest repository and how to use `west init --local` to initialize the workspace _locally_.
Instead of initializing a workspace _locally_, let’s use the manifest file and repository that we’ve created in the previous section and initialize the workspace from scratch using its remote URL.
```bash
mkdir workspace-m && cd workspace-m
west init -m https://github.com/lmapii/practical-zephyr-t2-empty-ws.git
```

```
.
├── app
│   ├── .git
│   ├── .gitignore
│   ├── readme.md
│   └── west.yml
└── .west
    └── config
```
When using the `--local` flag or the `-m` arguments:
- With `--local`, the _directory_ specifies the path to the local manifest repository.
- Without the `--local` flag, the _directory_ refers to the _topdir_ and thus the folder in which to create the workspace (defaulting to the current working directory in this case).
## Updating The Manifest Repository Path
If our use `west init` on a repository with no `manifest.self.path` defined, the name of the application cloned will be the name of the repository.
The manifest file uses the key `manifest.self` for configuring the manifest repository itself, meaning that all settings in the `manifest.self` key are only applied to the manifest repository. The key `manifest.self.path` can be used to specify the path that _West_ uses when cloning the manifest repository, relative to the _West_ workspace _topdir_.
```yml
self:
  path: <name_of_app>
```
Now if we use `west init` on our repo, the cloned app will have the name: `<name_of_app>`.
## Locally vs. Remotely Initialized Workspaces
The only difference is that for remotely initialized workspaces _West_ clones the repository for you and thus you essentially don’t need to use `git clone` to obtain the manifest repo used to setup the workspace.
```yml
[manifest]
path = app
file = west.yml
```
the `[manifest]` section points to the manifest file in the `app` folder. Running `west update` therefore only checks the contents of the local manifest file. It won’t try to pull new changes in the manifest repository and it also won’t attempt to read the file from the remote.
If there were any changes to the manifest file in the repository, you’ll have to `git pull` them in your manifest repository - which is a good thing. In fact, `west update` will never attempt to modify the manifest repository and also states this in the `--help` information for the `update` command:
> “This command does not alter the manifest repository’s contents.”
## Zephyr With West
### Application Skeleton
```
.
└── app
    ├── boards
    │   └── nrf52840dk_nrf52840.overlay
    ├── src
    │   └── main.c
    ├── CMakeLists.txt
    └── prj.conf
```
Having defined the contents of necessary files, if we run `west build`, we will get the following output:
```
usage: west [-h] [-z ZEPHYR_BASE] [-v] [-V] <command> ...
west: unknown command "build"; workspace /path/to/workspace-m does not define this extension command -- try "west help"
```
West `build` and `flash` are zephyr specific commands, and we don't have zephyr source code in our workspace yet.
### Setting Up The Manifest Repository and File
```
$ cd app
$ git init
$ touch west.yml
$ git add --all
$ cd ../

$ tree --dirsfirst -a -L 3
.
└── app  # manifest repository
    ├── .git
    ├── boards
    │   └── nrf52840dk_nrf52840.overlay
    ├── src
    │   └── main.c
    ├── CMakeLists.txt
    ├── prj.conf
    └── west.yml  # manifest file
```
Now we initialize our workspace as follows:
```bash
west init --local app
```
Now we can add the zephyr project in `manifest.projects` in the `west.yml` file.
```yml
manifest:
  version: 0.8

  self:
    path: app

  projects:
    - name: zephyr
      revision: v3.4.0
      url: https://github.com/zephyrproject-rtos/zephyr
      path: deps/zephyr
```
Running `west update` clones the Zephyr repository into `deps/zephyr` and checks out the commit with the tag `v3.4.0`.
Now, we have a complete “T2 star topology” workspace, where the Zephyr dependency is placed in a separate `deps` folder in the _topdir_
```
.  # topdir
├── .west
│   └── config
├── app  # manifest repository
│   ├── .git
│   ├── boards
│   │   └── nrf52840dk_nrf52840.overlay
│   ├── src
│   │   └── main.c
│   ├── CMakeLists.txt
│   ├── prj.conf
│   └── west.yml  # manifest file
└── deps
    └── zephyr
```
Now if we try to `west build --board <board>`, we will get the same error as before.
We can fix this by adding the `west-commands` key to the `zephyr` project: Zephyr’s _West_ extensions are provided by the file `scripts/west-commands.yml` in Zephyr’s repository. Using the key `west-commands`, we can provide a relative path to _West_ extension commands within the project:
`app/west.yml`
```yml
manifest:
  version: 0.8

  self:
    path: app

  projects:
    - name: zephyr
      revision: v3.4.0
      url: https://github.com/zephyrproject-rtos/zephyr
      path: deps/zephyr
      # explicitly add Zephyr-specific West extensions
      west-commands: scripts/west-commands.yml
```
We still can't successfully build because Zephyr builds require that a toolchain is either installed globally or specified using the `ZEPHYR_TOOLCHAIN_VARIANT` environment variable.
The term [“Zephyr SDK”](https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html) refers to the toolchains that must be provided for each of Zephyr’s supported architectures. With our manifest, we only add Zephyr’s _sources_ to our workspace. The required tools, however, e.g., the GCC compiler for ARM Cortex-M, are **not** included.
## Adding a Toolchain
West workspaces do **not** include the required _toolchain_ needed for building your application. This is something that you need to manage yourself.
- For pointing Zephyr’s build process to your installed SDK you can use the two environment variables [`ZEPHYR_TOOLCHAIN_VARIANT`](https://docs.zephyrproject.org/latest/develop/env_vars.html#envvar-ZEPHYR_TOOLCHAIN_VARIANT) and [`ZEPHYR_SDK_INSTALL_DIR`](https://docs.zephyrproject.org/latest/develop/env_vars.html#envvar-ZEPHYR_SDK_INSTALL_DIR).
- `ZEPHYR_TOOLCHAIN_VARIANT` is set to `zephyr` and `ZEPHYR_SDK_INSTALL_DIR` to the full path of the toolchain (SDK) installation.
to install all Python packages required by Zephyr. Instead of cloning Zephyr as documented in the [getting started guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html#install-dependencies), I can use the locally populated Zephyr installation:
```bash
pip install -r deps/zephyr/scripts/requirements.txt
```
Now we can `build`, but we see that the build process was now able to correctly determine the `host-tools` and `toolchains`, but the build still failed with an error message that is quite hard to comprehend.
## Manifest Imports
The build fails since we’re only adding the _Zephyr_ repository as a dependency. This is not enough: The Zephyr repository has its own dependencies, which it lists as _projects_ in its own `west.yml` file. We can _recursively import_ the required projects from Zephyr using the `import` key as follows:
`app/west.yml`
```yml
manifest:
  version: 0.8

  self:
    path: app

  projects:
    - name: zephyr
      revision: v3.4.0
      url: https://github.com/zephyrproject-rtos/zephyr
      import:
        path-prefix: deps
        file: west.yml
        name-allowlist:
          - cmsis
          - hal_nordic
```
We got rid of the `path` key, since `import.path-prefix` allows us to define a common prefix for all projects. Using the `import.file` key, we’re telling _West_ to look for a `west.yml` file in Zephyr’s repository and also consider the projects and West commands listed there. Notice that by default West looks for a `west.yml` file when using `import` and therefore it is not necessary to provide the `import.file` entry.
- Instead of adding _all_ of Zephyr’s dependencies, we pick the ones we need _by their name_ using the `import.name-allowlist` key.
Running `west update`, the new dependencies are placed in the `deps/modules` folder: We specified the `deps` prefix, whereas the `module` folder comes from Zephyr’s own `west.yml` file:
```
$ tree --dirsfirst -a -L 5
.
├── .west
│   └── config
├── app
│   ├── .git
│   ├── boards
│   │   └── nrf52840dk_nrf52840.overlay
│   ├── src
│   │   └── main.c
│   ├── CMakeLists.txt
│   ├── prj.conf
│   ├── setup-sdk-nrf.sh
│   └── west.yml
├── build  [13 entries exceeds filelimit, not opening dir]
└── deps
    ├── modules
    │   └── hal
    │       ├── cmsis
    │       └── nordic
    └── zephyr  [43 entries exceeds filelimit, not opening dir]
```
Now if we try to `build` it succeeds.
## Switching Boards
The first thing we need is an overlay file to specify the `/chosen` LED node:
`app/boards/nucleo_f411re.overlay`
```c
/ {
    chosen {
        app-led = &green_led_2;
    };
};
```
The STM32 HAL is part of the project `hal_stm32`, which we can now add to our allow-list in the manifest file:
```yml
 name-allowlist:
          - cmsis
          - hal_nordic
          - hal_stm32 # <-- added STM32 HAL
```
Running `west update`, _West_ populates the `stm32` HAL in the expected folder `deps/modules/hal/stm32`:
Building the application for the STM32 development kit now succeeds as expected:
```bash
west build --board nucleo_f411re app --pristine
```
## Working on Projects With West
**manifest-rev** **Branch**: For every project (Git repository) defined in your manifest file and cloned into your workspace, West creates and controls a **Git branch named** **manifest-rev**. When you `cd` into a project directory managed by West, you will find that you are on this `manifest-rev` branch.
• **Purpose of** **manifest-rev**: This branch is specifically maintained by West. It's essentially where West checks out the exact revision (branch, tag, or commit hash) specified for that project in your manifest file.
- **West's Control**: West completely controls this `manifest-rev` branch. It will **recreate and reset** the `manifest-rev` branch every time the `west update` command is executed.
- **Implications for Developers**:
- **Do Not Modify** **manifest-rev**: It is crucial **not to modify the** **manifest-rev** **branch** or push changes to it, as any changes you make will be lost upon the next `west update`.
- **Dedicated Branches for Work**: If you need to work on a project repository (e.g., make changes to a shared dependency), you should **create and use dedicated branches** within that repository.
- **Handling Local Changes**: West is designed to prevent accidental data loss. If you have local changes in a project's working tree (even if you haven't committed them) that conflict with an incoming update, `west update` will generate an error and abort, but your local changes will not be lost. Even if you mistakenly commit changes to the local `manifest-rev` branch, Git will issue a warning during `west update` indicating that a commit is "left behind," but it provides instructions on how to recover that commit by creating a new branch.
- **Post-Update Check**: After every `west update`, it's recommended to **double-check that you are still working on the correct branch** within your project repositories.
In contrast, West **does not alter the contents of the manifest repository** itself. If there are changes to the manifest file in the remote repository, you must manually perform a `git pull` within your manifest repository to get those updates.
---
#embedded #zephyr 