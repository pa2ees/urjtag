def promotionConfig = [
    // Mandatory parameters
    'targetRepo'         : 'fpga-deb-release',

    // Optional parameters

    // The build name and build number to promote. If not specified, the Jenkins job's build name and build number are used
    'buildName'          : 'urjtag',
    'buildNumber'        : env.BUILD_NUMBER,
    // Comment and Status to be displayed in the Build History tab in Artifactory
    'comment'            : 'this is the promotion comment',
    'status'             : 'Released',
    // Specifies the source repository for build artifacts.
    'sourceRepo'         : 'fpga-deb-nightly',
    // Indicates whether to promote the build dependencies, in addition to the artifacts. False by default
    //'includeDependencies': true,
    // Indicates whether to copy the files. Move is the default
    'copy'               : true,
    // Indicates whether to fail the promotion process in case of failing to move or copy one of the files. False by default.
    'failFast'           : true
]

node('merlin') {
  stage('Checkout') {
    cleanWs()
    checkout scm
    sh "[ -f ubuntu-ruby ] || wget -qN https://hub.docker.com/r/travisci/ubuntu-ruby"
  }

  def workdir = pwd()
  def MAJOR = 0
  def MINOR = 0
  def PATCH = 1

  docker.withRegistry('https://embedded-docker-release.artifactory.imsar.us') {
    docker.image('travisci_ubuntu-ruby_16.04_armv7l').inside("--workdir=${workdir}") {
      stage('Build armv7l') {
        sh "MAJOR=${MAJOR} MINOR=${MINOR} PATCH=${PATCH} BUILD_NUMBER=${env.BUILD_NUMBER} ci/build-armv7l.sh && \
cp urjtag/imsar-urjtag_*.*.*-*_*.deb ."
      }
    }
  }

  docker.withRegistry('https://embedded-docker-release.artifactory.imsar.us') {
    docker.image('travisci_ubuntu-ruby_16.04_aarch64').inside("--workdir=${workdir}") {
      stage('Build aarch64') {
        sh "MAJOR=${MAJOR} MINOR=${MINOR} PATCH=${PATCH} BUILD_NUMBER=${env.BUILD_NUMBER} ci/build-aarch64.sh && \
cp urjtag/imsar-urjtag_*.*.*-*_*.deb ."
      }
    }
  }

  docker.withRegistry('https://embedded-docker-release.artifactory.imsar.us') {
    docker.image('travisci_ubuntu-ruby_16.04_x86_64').inside("--workdir=${workdir}") {
      stage('Build x86_64') {
        sh "MAJOR=${MAJOR} MINOR=${MINOR} PATCH=${PATCH} BUILD_NUMBER=${env.BUILD_NUMBER} ci/build-x86_64.sh && \
cp urjtag/imsar-urjtag_*.*.*-*_*.deb ."
      }
    }
  }

  stage('Archive') {
    archiveArtifacts artifacts: 'imsar-urjtag_*.*.*-*_*.deb', fingerprint: true
  }
  if(env.BRANCH_NAME == "master") {
    stage('Upload')
    {
      def uploadSpec = """{
        "files": [
          {
            "pattern": "imsar-urjtag_*.*.*-*_amd64.deb",
            "target": "fpga-deb-nightly/pool/urjtag/",
            "props": "deb.distribution=xenial;deb.component=contrib;deb.architecture=amd64"
          },
          {
            "pattern": "imsar-urjtag_*.*.*-*_armhf.deb",
            "target": "fpga-deb-nightly/pool/urjtag/",
            "props": "deb.distribution=xenial;deb.component=contrib;deb.architecture=armhf"
          },
          {
            "pattern": "imsar-urjtag_*.*.*-*_arm64.deb",
            "target": "fpga-deb-nightly/pool/urjtag/",
            "props": "deb.distribution=xenial;deb.component=contrib;deb.architecture=arm64"
          }
        ]
      }"""
      def server = Artifactory.server 'artifactory'
      def buildInfo = Artifactory.newBuildInfo()
      buildInfo.env.capture = true
      buildInfo.env.collect()
      buildInfo.name = 'urjtag'
      buildInfo.retention maxBuilds: 10, deleteBuildArtifacts: true, async: true
      server.upload spec: uploadSpec, buildInfo: buildInfo
      server.publishBuildInfo buildInfo

      Artifactory.addInteractivePromotion server: server, promotionConfig: promotionConfig, displayName: "Promote me please"
    }
  }
}
