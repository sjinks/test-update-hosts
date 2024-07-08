import { stat, chmod } from 'node:fs/promises';
import { createGunzip } from 'node:zlib';
import { pipeline } from 'node:stream/promises';
import { createWriteStream } from 'node:fs';
import { spawn } from 'node:child_process';

const exists = (filename) => stat(filename).then((s) => s.isFile()).catch(() => false);

const archMap = {
    ia32: '386',
    x64: 'amd64',
    arm64: 'arm64',
};

const platformMap = {
    win32: 'windows',
    darwin: 'sarwin',
    linux: 'linux',
};

const { arch, platform } = process;
const resolvedArch = archMap[arch];
const resolvedPlatform = platformMap[platform];

if (!resolvedArch || !resolvedPlatform) {
    throw new Error('Unsupported platform or architecture');
}

const suffix = 'windows' === resolvedPlatform ? '.exe' : '';
const filename = `update-hosts${suffix}`;

if (!await exists(filename)) {
    const url = new URL(`https://github.com/sjinks/test-update-hosts/releases/latest/download/dev-env-update-hosts-${resolvedPlatform}-${resolvedArch}${suffix}.gz`);

    const response = await fetch(url);
    if (!response.ok || !response.body) {
        throw new Error(`Failed to download ${url}`);
    }

    const gunzipStream = createGunzip();
    const fileStream = createWriteStream(`update-hosts${suffix}`);
    await pipeline(response.body, gunzipStream, fileStream);
    await chmod(`update-hosts${suffix}`, 0o755);
}

const child = spawn(`./${filename}`, process.argv.slice(2), { stdio: 'inherit' });
child.on('exit', (code) => process.exit(code));
child.on('error', (err) => {
    console.error(err);
    process.exit(1);
});
