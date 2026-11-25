import type { Metadata, Viewport } from "next"
import "@fontsource-variable/inter"
import "@fontsource/ibm-plex-mono/latin-400.css"
import "@fontsource/ibm-plex-mono/latin-500.css"
import "./globals.css"
import { Provider } from "@/components/ui/provider"
import { Toaster } from "@/components/ui/toaster"

const description =
  "Konsol kontrol mixer audio ESP32-P4: delapan kanal, EQ 15 band, dan bus Main, Group, serta Aux."

export const metadata: Metadata = {
  metadataBase: new URL(process.env.NEXT_PUBLIC_SITE_URL ?? "http://192.168.4.1"),
  title: "P4 Digital Mixer — Console",
  description,
  applicationName: "P4 Digital Mixer",
  icons: {
    icon: "/icon.svg",
  },
  openGraph: {
    title: "P4 Digital Mixer",
    description,
    type: "website",
    images: [
      {
        url: "/share.svg",
        width: 1200,
        height: 630,
        alt: "P4 Digital Mixer console",
      },
    ],
  },
  twitter: {
    card: "summary_large_image",
    title: "P4 Digital Mixer",
    description,
    images: ["/share.svg"],
  },
}

export const viewport: Viewport = {
  width: "device-width",
  initialScale: 1,
  viewportFit: "cover",
  themeColor: "#FFFFFF",
}

export default function RootLayout({
  children,
}: Readonly<{ children: React.ReactNode }>) {
  return (
    <html lang="id" suppressHydrationWarning>
      <body>
        <Provider>
          {children}
          <Toaster />
        </Provider>
      </body>
    </html>
  )
}
