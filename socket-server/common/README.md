# C++ 공통 코드

이 폴더는 C++ 소켓 서버와 mockclient의 프레이밍·직렬화·네트워크 기본값·콘솔 유틸리티를 소유합니다.
Unity와 공유하는 통신 원본은 [contracts/realtime](../../contracts/realtime/README.md)입니다.

- `include/MessageCode.hpp`, `include/ErrorCode.hpp`: `codes.json`에서 생성하며 직접 편집하지 않습니다.
- `include/PacketHeader.hpp`, `include/Packet.hpp`, `include/ProtobufCodec.hpp`: C++ 패킷 구현.
- `include/ProtocolLimits.hpp`: C++ 프레임·본문·이전 설계의 제한값.
- `include/NetworkSettings.hpp`: C++ 실행 프로그램의 기본 포트·큐 설정.
- `src/ConsoleExit.cpp`: 개발용 콘솔 종료 처리.

CMake가 공통 `.proto`에서 C++ 코드를 빌드 디렉터리에 생성합니다.
루트의 `tools/Generate-Protocol.ps1`은 같은 원본에서 Unity 메시지와 양쪽 코드 상수를 생성합니다.
루트의 `tools/Test.ps1`로 두 언어의 실제 통신과 생성 결과를 검증합니다.
