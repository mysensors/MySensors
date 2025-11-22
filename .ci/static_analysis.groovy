#!groovy

def cppCheck(config) {
    config.pr.setBuildStatus(
        config,
        'PENDING',
        'Toll gate (Code analysis - Cppcheck)',
        'Running...',
        "${BUILD_URL}flowGraphTable/"
    )

    sh """#!/bin/bash
set -euo pipefail

cd "${config.repository_root}"

echo "Doing cppcheck for AVR..."

# Collect C/C++/INO files and run cppcheck
find . -type f \\( -iname '*.c' -o -iname '*.cpp' -o -iname '*.ino' \\) \
    | cppcheck -j 4 \
        --force \
        --file-list=- \
        --enable=style,portability,performance \
        --platform=.mystools/cppcheck/config/avr.xml \
        --suppressions-list=.mystools/cppcheck/config/suppressions.cfg \
        --includes-file=.mystools/cppcheck/config/includes.cfg \
        --language=c++ \
        --inline-suppr \
        --xml --xml-version=2 \
        2> cppcheck-avr.xml

cppcheck-htmlreport \
    --file="cppcheck-avr.xml" \
    --title="cppcheck-avr" \
    --report-dir=cppcheck-avr_cppcheck_reports \
    --source-dir=.
"""

    // Ensure repository_root ends with a slash
    def root = config.repository_root.endsWith('/') ? config.repository_root : config.repository_root + '/'

    publishHTML([
        allowMissing: false,
        alwaysLinkToLastBuild: false,
        keepAll: true,
        reportDir: root + 'cppcheck-avr_cppcheck_reports',
        reportFiles: 'index.html',
        reportName: 'CppCheck AVR',
        reportTitles: ''
    ])

    // Only run ViolationsToGitHubRecorder if we have a PR id
    if (env.CHANGE_ID) {
        step([
            $class: 'ViolationsToGitHubRecorder',
            config: [
                repositoryName: config.repository_name,
                pullRequestId: env.CHANGE_ID,
                createCommentWithAllSingleFileComments: true,
                createSingleFileComments: true,
                commentOnlyChangedContent: true,
                keepOldComments: false,
                violationConfigs: [[
                    pattern: '.*/cppcheck-avr\\.xml$',
                    parser: 'CPPCHECK',
                    reporter: 'Cppcheck'
                ]]
            ]
        ])
    }

    def ret = sh(
        returnStatus: true,
        script: """#!/bin/bash
cd "${config.repository_root}"

if ! [ -f cppcheck-avr_cppcheck_reports/index.html ]; then
    echo "cppcheck HTML report not found!"
    exit 1
fi

if grep -q "0</td><td>total" cppcheck-avr_cppcheck_reports/index.html; then
    exit 0
else
    exit 1
fi
"""
    )

    if (ret == 1) {
        config.pr.setBuildStatus(
            config,
            'ERROR',
            'Toll gate (Code analysis - Cppcheck)',
            'Issues found',
            "${BUILD_URL}CppCheck_20AVR/index.html"
        )
        error 'Terminating due to Cppcheck error'
    } else {
        config.pr.setBuildStatus(
            config,
            'SUCCESS',
            'Toll gate (Code analysis - Cppcheck)',
            'Pass',
            ''
        )
    }
}

return this
