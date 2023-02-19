const {JSDOM} = require('jsdom');
const {copyFile, mkdir, readdir, stat, writeFile} = require('node:fs/promises');
const {dirname, extname, relative, resolve} = require('node:path');
const {argv, exit} = require('node:process');

/**
 * Recursively gets file paths in a directory
 * @async
 * @generator
 * @function
 * @param {string} dir - The directory to get files from
 * @returns {AsyncIterable<string>} - Yields file paths as they are found
 */
async function* getFiles(dir) {
    const dirents = await readdir(dir, {withFileTypes: true});
    for (const dirent of dirents) {
        const path = resolve(dir, dirent.name);
        if (dirent.isDirectory()) {
            yield* getFiles(path);
        } else if (dirent.isFile()) {
            yield path;
        }
    }
}

/**
 * Returns the result of executing JavaScript in an HTML file
 * @function
 * @param {string} file - The HTML file to execute JavaScript in
 * @returns {Promise<string>} - The outerHTML of the document after JavaScript has executed
 */
function applyJS(file) {
    return new Promise((resolve) => {
        JSDOM.fromFile(file, {
            resources: 'usable',
            runScripts: 'dangerously',
            beforeParse: (window) => {
                window.addEventListener('load', () => {
                    resolve(window.document.documentElement.outerHTML);
                });
            },
        });
    });
}

/**
 * Checks if a given file exists and is a directory.
 *
 * @async
 * @function
 * @param {string} file - The file path to check.
 * @returns {Promise<boolean>} - True if the file exists and is a directory, false otherwise.
 */
async function isDir(file) {
    try {
        return (await stat(file)).isDirectory();
    } catch (error) {
        return false;
    }
}

async function main() {
    if (argv.length < 4) {
        console.log(`usage: node ${argv[1]} source_dir target_dir`);
        exit(1);
    }

    const sourceDir = argv[2];
    const targetDir = argv[3];

    if (!(await isDir(sourceDir))) {
        console.log(`${sourceDir} is not a valid directory`);
        exit(1);
    }

    if (!(await isDir(targetDir))) {
        console.log(`${targetDir} is not a valid directory`);
        exit(1);
    }

    for await (const file of getFiles(sourceDir)) {
        switch (extname(file)) {
            case '.html': {
                const targetPath = resolve(targetDir, relative(sourceDir, file));
                await mkdir(dirname(targetPath), {recursive: true});
                const resultHTML = await applyJS(file);
                await writeFile(targetPath, resultHTML, 'utf-8');
                break;
            }
            case '.css': {
                const targetPath = resolve(targetDir, relative(sourceDir, file));
                await mkdir(dirname(targetPath), {recursive: true});
                await copyFile(file, targetPath);
                break;
            }
            default: {
                break;
            }
        }
    }
}

main().catch((error) => {
    console.log(error);
});
